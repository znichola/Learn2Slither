port module Main exposing (main)

import App.Board exposing (..)
import App.Config exposing (..)
import Array exposing (Array)
import Browser
import Html exposing (Html, button, div, h3, text)
import Html.Attributes exposing (class)
import Html.Events exposing (onClick, preventDefaultOn)
import Json.Decode as Decode
import Json.Encode as Encode
import Time



-- MODEL & TYPES


type Mode
    = Manual
    | AI
    | Training


type alias ActiveSession =
    { mode : Mode
    , batch : List Board
    }


type alias Run =
    { id : Int
    , message : String
    , frames : Array Board
    , playbackIndex : Int
    }


type LogType
    = Error String
    | Log String
    | RunLog Run


type alias Model =
    { board : Maybe Board
    , logs : List LogType
    , activeSession : Maybe ActiveSession
    , replayingRun : Maybe Run
    , nextRunId : Int
    , config : Config
    , qtableSize : Int
    }



-- INIT & CONFIG


init : () -> ( Model, Cmd Msg )
init _ =
    ( { board = Nothing
      , logs = []
      , activeSession = Nothing
      , replayingRun = Nothing
      , nextRunId = 1
      , config = defaultConfig
      , qtableSize = 0
      }
    , Cmd.none
    )


defaultConfig : Config
defaultConfig =
    { episodes = fieldInt 1000
    , samplePerReplay = fieldInt 100 |> updateFieldHint "Samples before training animation is played."
    , maxSteps = fieldInt 1000 |> updateFieldHint "Max steps per episode before stop."
    , frameTimeMs = fieldInt 30 |> updateFieldHint "During playback the delay between frames in ms."
    , boardX = fieldInt 10
    , boardY = fieldInt 10
    , alpha = fieldFloat 0.1 |> updateFieldHint "Learning rate. When lower, it's slower but more stable. (0.0 - 1.0)"
    , gamma = fieldFloat 0.995 |> updateFieldHint "Discount factor for future rewards. When higher, the model prioritises future rewards more, but too high and the snake may loop to stay alive rather than risk seeking food. (0.0 - 1.0)"
    , epsilon = fieldFloat 0.8 |> updateFieldHint "Exploration rate. At 1.0 it will always take random actions. (0.0 - 1.0)"
    , epsilonDecay = fieldFloat 0.9925 |> updateFieldHint "Multiplied with epsilon each episode. When lower, exploration drops off faster. Set to 0.0 to remove decay."
    , epsilonMin = fieldFloat 0.05 |> updateFieldHint "Floor for epsilon when using epsilon decay."
    , rewardAdvance = fieldFloat -0.01
    , rewardGreen = fieldFloat 10.0
    , rewardRed = fieldFloat -2.0
    , rewardDeath = fieldFloat -10.0
    , stateFn = DistToClosest
    , stateInit = InstantDeath
    }



-- EPSILON ESTIMATION


episodesToReachMin : Config -> Maybe Int
episodesToReachMin config =
    let
        epsilonMin =
            getField config.epsilonMin + 0.001
    in
    if getField config.epsilon <= epsilonMin then
        Just 0

    else if getField config.epsilonDecay <= 0 then
        Just 1

    else if getField config.epsilonDecay >= 1 || epsilonMin <= 0 then
        Nothing

    else
        Just (ceiling (logBase (getField config.epsilonDecay) (epsilonMin / getField config.epsilon)))


epsilonDecayHint : Config -> String
epsilonDecayHint config =
    let
        base =
            "Multiplied with epsilon each episode. When lower, exploration drops off faster. Set to 0.0 to remove decay. "
    in
    case episodesToReachMin config of
        Just n ->
            base ++ "At current settings, epsilon would reach ~" ++ String.fromFloat (getField config.epsilonMin) ++ " after " ++ String.fromInt n ++ " episodes."

        Nothing ->
            base ++ "At current settings, epsilon will never reach its minimum."


withComputedHints : Config -> Config
withComputedHints config =
    { config | epsilonDecay = updateFieldHint (epsilonDecayHint config) config.epsilonDecay }



-- PORTS


port sendToJs : Encode.Value -> Cmd msg


port receiveFromJs : (Decode.Value -> msg) -> Sub msg



-- UPDATE & WASM MESSAGES


type Msg
    = SendStep String
    | ToggleSnakeVision
    | StartManual
    | StartAI
    | StartTrain
    | SendStopTrain
    | SendTrainMore
    | GotWasmMessage Decode.Value
    | ReplayTick Time.Posix
    | UpdateConfig ConfigField String
    | SendSaveAgent
    | SendLoadAgent
    | ReplayRun Run


type alias WasmMessage =
    { msgType : String
    , source : String
    , content : String
    }


decodeWasmMessage : Decode.Value -> Result Decode.Error WasmMessage
decodeWasmMessage =
    Decode.decodeValue
        (Decode.map3 WasmMessage
            (Decode.field "type" Decode.string)
            (Decode.field "source" Decode.string)
            (Decode.field "content" Decode.string)
        )


applyWasmMessage : WasmMessage -> Model -> Model
applyWasmMessage wasmMsg model =
    case wasmMsg.msgType of
        "board" ->
            case parseBoard wasmMsg.content of
                Just board ->
                    { model
                        | activeSession = Maybe.map (\s -> { s | batch = board :: s.batch }) model.activeSession
                        , board =
                            if model.replayingRun == Nothing then
                                Just board

                            else
                                model.board
                    }

                Nothing ->
                    model

        "run_done" ->
            handleRunDone wasmMsg.content model

        "log" ->
            { model | logs = Log wasmMsg.content :: model.logs }

        "error" ->
            { model | logs = Error wasmMsg.content :: model.logs }

        "save_agent" ->
            { model | logs = Log "new agent save" :: model.logs }

        "qtable_size" ->
            { model | qtableSize = wasmMsg.content |> String.trim |> String.toInt |> Maybe.withDefault 0 }

        "push_config" ->
            case decodeConfig wasmMsg.content of
                -- TODO ensure it's only on agent load that we push config
                Ok config ->
                    { model | config = config, activeSession = Nothing }

                Err err ->
                    { model | logs = Error ("Failed to decode config: " ++ Decode.errorToString err) :: model.logs }

        _ ->
            { model | logs = Error ("unknown message type: " ++ wasmMsg.msgType) :: model.logs }


handleRunDone : String -> Model -> Model
handleRunDone rawMessage model =
    let
        batch =
            model.activeSession |> Maybe.map .batch |> Maybe.withDefault []

        framesArray =
            Array.fromList (List.reverse batch)

        ( newLogEntry, maybeRun ) =
            if Array.isEmpty framesArray then
                ( Log rawMessage, Nothing )

            else
                let
                    run =
                        { id = model.nextRunId
                        , message = rawMessage
                        , frames = framesArray
                        , playbackIndex = 0
                        }
                in
                if String.contains "GameOver" rawMessage then
                    ( RunLog run, Nothing )

                else
                    ( RunLog run, Just run )

        ( nextRunId, nextBoard ) =
            case maybeRun of
                Just run ->
                    ( model.nextRunId + 1, Array.get 0 run.frames )

                Nothing ->
                    ( model.nextRunId, model.board )

        continueTraining =
            String.contains "Epoch summary" rawMessage

        -- When a trainng session is done, the string containes Traing Complete
        -- When an AI game is done, it's AI
        -- Continuous training keeps the active session alive
        nextSession =
            case model.activeSession of
                Just { mode } ->
                    if mode == Training && continueTraining then
                        Just { mode = Training, batch = [] }

                    else
                        Nothing

                Nothing ->
                    Nothing
    in
    { model
        | logs = newLogEntry :: model.logs
        , activeSession = nextSession
        , replayingRun = maybeRun
        , board = nextBoard
        , nextRunId = nextRunId
    }


update : Msg -> Model -> ( Model, Cmd Msg )
update msg model =
    case msg of
        ToggleSnakeVision ->
            ( model
            , sendToJs (Encode.object [ ( "type", Encode.string "STEP" ), ( "value", Encode.string "Toggle snake vision" ) ])
            )

        SendStep value ->
            ( model
            , sendToJs (Encode.object [ ( "type", Encode.string "STEP" ), ( "value", Encode.string value ) ])
            )

        StartManual ->
            let
                payload =
                    Encode.object
                        [ ( "type", Encode.string "MANUAL" )
                        , ( "value"
                          , Encode.object
                                [ ( "board_x", Encode.int (getField model.config.boardX) )
                                , ( "board_y", Encode.int (getField model.config.boardY) )
                                ]
                          )
                        ]
            in
            ( { model | activeSession = Just { mode = Manual, batch = [] }, replayingRun = Nothing }
            , sendToJs payload
            )

        StartAI ->
            ( { model | activeSession = Just { mode = AI, batch = [] }, replayingRun = Nothing, logs = Log "Sent AI play command to WASM" :: model.logs }
            , sendToJs (Encode.object [ ( "type", Encode.string "AI" ), ( "value", encodeConfig model.config ) ])
            )

        StartTrain ->
            ( { model | activeSession = Just { mode = Training, batch = [] }, nextRunId = 0, replayingRun = Nothing, logs = [ Log "Sent train command to WASM" ] }
            , sendToJs (Encode.object [ ( "type", Encode.string "TRAIN" ), ( "value", encodeConfig model.config ) ])
            )

        SendTrainMore ->
            ( { model | activeSession = Just { mode = Training, batch = [] }, replayingRun = Nothing, logs = Log "Sent train more command to WASM" :: model.logs }
            , sendToJs (Encode.object [ ( "type", Encode.string "RESUME_TRAIN" ), ( "value", encodeConfig model.config ) ])
            )

        SendStopTrain ->
            ( { model | activeSession = Nothing, replayingRun = Nothing }, Cmd.none )

        GotWasmMessage value ->
            case decodeWasmMessage value of
                Ok wasmMsg ->
                    ( applyWasmMessage wasmMsg model, Cmd.none )

                Err _ ->
                    ( { model | logs = Error "Failed to decode WASM message" :: model.logs }, Cmd.none )

        UpdateConfig field str ->
            ( { model | config = updateConfig field str model.config }, Cmd.none )

        SendSaveAgent ->
            ( { model | logs = [ Log "Sent AgentSave command to WASM" ] }
            , sendToJs (Encode.object [ ( "type", Encode.string "SAVE_AGENT" ), ( "value", Encode.string "save agent state" ) ])
            )

        SendLoadAgent ->
            ( { model | logs = [ Log "Sent AgentLoad command to WASM" ], activeSession = Nothing, replayingRun = Nothing }
            , sendToJs (Encode.object [ ( "type", Encode.string "LOAD_AGENT" ), ( "value", Encode.string "load agent state" ) ])
            )

        ReplayRun run ->
            ( { model | board = Array.get 0 run.frames, replayingRun = Just { run | playbackIndex = 0 } }, Cmd.none )

        ReplayTick _ ->
            case model.replayingRun of
                Just run ->
                    let
                        nextIndex =
                            run.playbackIndex + 1
                    in
                    if nextIndex >= Array.length run.frames then
                        let
                            cmd =
                                case model.activeSession of
                                    Just { mode } ->
                                        if mode == Training then
                                            sendToJs (Encode.object [ ( "type", Encode.string "RESUME_TRAIN" ), ( "value", Encode.string "continue" ) ])

                                        else
                                            Cmd.none

                                    Nothing ->
                                        Cmd.none
                        in
                        ( { model | replayingRun = Nothing }, cmd )

                    else
                        ( { model | board = Array.get nextIndex run.frames, replayingRun = Just { run | playbackIndex = nextIndex } }, Cmd.none )

                Nothing ->
                    ( model, Cmd.none )



-- VIEW


view : Model -> Html Msg
view model =
    div [ class "container" ]
        [ div [ class "game-section", preventDefaultOn "keydown" arrowKeyDecoder ]
            [ h3 [] [ text "Game Board" ]
            , div [ class "inner-board" ]
                [ div [ class "board-column" ]
                    [ viewBoard model.board
                    , viewControls
                    ]
                , viewBoardOptions model
                ]
            ]
        , div [ class "info-section" ]
            [ viewLogs model.replayingRun model.logs ]
        , div [ class "input-section" ]
            [ viewAppControl model
            , viewConfig (withComputedHints model.config) UpdateConfig
            ]
        ]


viewBoardOptions : Model -> Html Msg
viewBoardOptions model =
    div [ class "board-options" ]
        [ div [] [ text "Board Options" ]
        , button [ class "option-btn", onClick ToggleSnakeVision ] [ text "Toggle snake vision" ]
        , viewIntField "Frame Time (ms)" FrameTime model.config.frameTimeMs UpdateConfig
        , div [ class "option-item qtable-stat" ] [ text ("Q-table size: " ++ String.fromInt model.qtableSize) ]
        ]


viewStatusDetails : Model -> Html Msg
viewStatusDetails model =
    div [ class "status-details" ]
        [ button [ onClick ToggleSnakeVision ] [ text "toggle snake vision" ]
        , div [] [ text ("Qtable size: " ++ String.fromInt model.qtableSize) ]
        ]


viewControls : Html Msg
viewControls =
    div [ class "controles" ]
        [ button [ onClick (SendStep "UP") ] [ text "up" ]
        , button [ onClick (SendStep "DOWN") ] [ text "down" ]
        , button [ onClick (SendStep "LEFT") ] [ text "left" ]
        , button [ onClick (SendStep "RIGHT") ] [ text "right" ]
        ]


viewAppControl : Model -> Html Msg
viewAppControl model =
    let
        isTraining =
            case model.activeSession of
                Just { mode } ->
                    mode == Training

                Nothing ->
                    False
    in
    div [ class "control-buttons", preventDefaultOn "keydown" arrowKeyDecoder ]
        [ button [ onClick StartManual ] [ text "Manual play" ]
        , button [ onClick StartAI ] [ text "AI play" ]
        , if isTraining then
            button [ onClick SendStopTrain ] [ text "Stop Training" ]

          else if model.qtableSize /= 0 then
            button [ onClick SendTrainMore ] [ text "Train More" ]

          else
            button [ onClick StartTrain ] [ text "Train" ]
        , if not isTraining && model.qtableSize /= 0 then
            button [ onClick StartTrain ] [ text "Train Fresh" ]

          else
            text ""
        , button [ onClick SendLoadAgent ] [ text "Load model" ]
        , button [ onClick SendSaveAgent ] [ text "Save model" ]
        ]


viewLogs : Maybe Run -> List LogType -> Html Msg
viewLogs replayingRun logs =
    div [ class "logs-container" ]
        [ h3 [] [ text "Logs" ]
        , div [ class "logs" ]
            (if List.isEmpty logs then
                [ text "No logs yet" ]

             else
                List.map (viewLog replayingRun) logs
            )
        ]


viewLog : Maybe Run -> LogType -> Html Msg
viewLog replayingRun entry =
    case entry of
        Log message ->
            div [ class "log-entry" ] [ text message ]

        Error message ->
            div [ class "error-entry" ] [ text message ]

        RunLog run ->
            let
                isCurrentlyReplaying =
                    case replayingRun of
                        Just activeRun ->
                            activeRun.id == run.id

                        Nothing ->
                            False

                activeClass =
                    if isCurrentlyReplaying then
                        " active-replay"

                    else
                        " active-replay__finished"
            in
            button
                [ class ("log-entry run-log-entry" ++ activeClass)
                , onClick (ReplayRun run)
                ]
                [ text ("▶ [Run #" ++ String.fromInt run.id ++ "] " ++ run.message) ]


keyToDirection : String -> Maybe String
keyToDirection rawKey =
    let
        key =
            String.toLower rawKey

        mappings =
            [ ( [ "arrowup", "k", "w" ], "UP" )
            , ( [ "arrowdown", "j", "s" ], "DOWN" )
            , ( [ "arrowleft", "h", "a" ], "LEFT" )
            , ( [ "arrowright", "l", "d" ], "RIGHT" )
            ]
    in
    mappings
        |> List.filter (\( keys, _ ) -> List.member key keys)
        |> List.head
        |> Maybe.map Tuple.second


arrowKeyDecoder : Decode.Decoder ( Msg, Bool )
arrowKeyDecoder =
    Decode.field "key" Decode.string
        |> Decode.andThen
            (\key ->
                case keyToDirection key of
                    Just direction ->
                        Decode.succeed ( SendStep direction, True )

                    Nothing ->
                        Decode.fail "not an arrow key"
            )



-- SUBSCRIPTIONS


subscriptions : Model -> Sub Msg
subscriptions model =
    Sub.batch
        [ receiveFromJs GotWasmMessage
        , case model.replayingRun of
            Just _ ->
                Time.every (toFloat (getField model.config.frameTimeMs)) ReplayTick

            Nothing ->
                Sub.none
        ]



-- MAIN


main : Program () Model Msg
main =
    Browser.element
        { init = init
        , view = view
        , update = update
        , subscriptions = subscriptions
        }
