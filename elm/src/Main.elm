port module Main exposing (main)

import Browser
import Html exposing (Html, button, div, h3, input, label, span, table, tbody, td, text, tr)
import Html.Attributes exposing (class, step, type_, value)
import Html.Events exposing (onClick, onInput, preventDefaultOn)
import Json.Decode as Decode
import Json.Encode as Encode
import Time



-- MODEL


type alias Model =
    { board : Maybe Board
    , logs : List LogType
    , receivedMessages : List WasmMessage
    , replayIndex : Maybe Int
    , config : Config
    , training : Bool
    }


type LogType
    = Error String
    | Log String


type alias Board =
    { rows : List (List Cell) }


type Cell
    = Wall
    | Empty
    | Snake
    | Head
    | Green
    | Red


init : () -> ( Model, Cmd Msg )
init _ =
    ( { board = Nothing
      , logs = []
      , receivedMessages = []
      , replayIndex = Nothing
      , config = defaultConfig
      , training = False
      }
    , Cmd.none
    )


type alias Field a =
    { raw : String
    , parsed : Maybe a
    , default : a
    }


fieldInt : Int -> Field Int
fieldInt n =
    { raw = String.fromInt n, parsed = Just n, default = n }


fieldFloat : Float -> Field Float
fieldFloat n =
    { raw = String.fromFloat n, parsed = Just n, default = n }


updateFieldInt : String -> Field Int -> Field Int
updateFieldInt str f =
    { raw = str, parsed = String.toInt str, default = f.default }


updateFieldFloat : String -> Field Float -> Field Float
updateFieldFloat str f =
    { raw = str, parsed = String.toFloat str, default = f.default }


getField : Field a -> a
getField field =
    Maybe.withDefault field.default field.parsed


type alias Config =
    { episodes : Field Int
    , batchSize : Field Int
    , maxSteps : Field Int
    , frameTimeMs : Field Int
    , boardX : Field Int
    , boardY : Field Int
    , alpha : Field Float
    , gamma : Field Float
    , epsilon : Field Float
    , epsilonDecay : Field Float
    , epsilonMin : Field Float
    , rewardAdvance : Field Float
    , rewardGreen : Field Float
    , rewardRed : Field Float
    , rewardDeath : Field Float
    }


defaultConfig : Config
defaultConfig =
    { episodes = fieldInt 10
    , batchSize = fieldInt 1
    , maxSteps = fieldInt 100
    , frameTimeMs = fieldInt 100
    , boardX = fieldInt 10
    , boardY = fieldInt 10
    , alpha = fieldFloat 0.4
    , gamma = fieldFloat 0.9
    , epsilon = fieldFloat 0.1
    , epsilonDecay = fieldFloat 0.995
    , epsilonMin = fieldFloat 0.01
    , rewardAdvance = fieldFloat 0.0
    , rewardGreen = fieldFloat 10000.0
    , rewardRed = fieldFloat 1.0
    , rewardDeath = fieldFloat -10.0
    }


type ConfigField
    = Episodes
    | BatchSize
    | MaxSteps
    | FrameTime
    | BoardX
    | BoardY
    | Alpha
    | Gamma
    | Epsilon
    | EpsilonDecay
    | EpsilonMin
    | RewardAdvance
    | RewardGreen
    | RewardRed
    | RewardDeath


updateConfig : ConfigField -> String -> Config -> Config
updateConfig field str config =
    case field of
        Episodes ->
            { config | episodes = updateFieldInt str config.episodes }

        BatchSize ->
            { config | batchSize = updateFieldInt str config.batchSize }

        MaxSteps ->
            { config | maxSteps = updateFieldInt str config.maxSteps }

        FrameTime ->
            { config | frameTimeMs = updateFieldInt str config.frameTimeMs }

        BoardX ->
            { config | boardX = updateFieldInt str config.boardX }

        BoardY ->
            { config | boardY = updateFieldInt str config.boardY }

        Alpha ->
            { config | alpha = updateFieldFloat str config.alpha }

        Gamma ->
            { config | gamma = updateFieldFloat str config.gamma }

        Epsilon ->
            { config | epsilon = updateFieldFloat str config.epsilon }

        EpsilonDecay ->
            { config | epsilonDecay = updateFieldFloat str config.epsilonDecay }

        EpsilonMin ->
            { config | epsilonMin = updateFieldFloat str config.epsilonMin }

        RewardAdvance ->
            { config | rewardAdvance = updateFieldFloat str config.rewardAdvance }

        RewardGreen ->
            { config | rewardGreen = updateFieldFloat str config.rewardGreen }

        RewardRed ->
            { config | rewardRed = updateFieldFloat str config.rewardRed }

        RewardDeath ->
            { config | rewardDeath = updateFieldFloat str config.rewardDeath }


encodeConfig : Config -> Encode.Value
encodeConfig config =
    Encode.object
        [ ( "EPISODES", Encode.int (getField config.episodes) )
        , ( "BATCH_SIZE", Encode.int (getField config.batchSize) )
        , ( "MAX_STEPS", Encode.int (getField config.maxSteps) )
        , ( "frame_time_ms", Encode.int (getField config.frameTimeMs) )
        , ( "board_x", Encode.int (getField config.boardX) )
        , ( "board_y", Encode.int (getField config.boardY) )
        , ( "alpha", Encode.float (getField config.alpha) )
        , ( "gamma", Encode.float (getField config.gamma) )
        , ( "epsilon", Encode.float (getField config.epsilon) )
        , ( "epsilon_decay", Encode.float (getField config.epsilonDecay) )
        , ( "epsilon_min", Encode.float (getField config.epsilonMin) )
        , ( "reward_advance", Encode.float (getField config.rewardAdvance) )
        , ( "reward_green", Encode.float (getField config.rewardGreen) )
        , ( "reward_red", Encode.float (getField config.rewardRed) )
        , ( "reward_death", Encode.float (getField config.rewardDeath) )
        ]



-- PORTS


port sendToJs : Encode.Value -> Cmd msg


port receiveFromJs : (Decode.Value -> msg) -> Sub msg



-- UPDATE


type Msg
    = SendStep String
    | SendManual
    | SendTrain Encode.Value
    | SendStopTrain
    | SendAI
    | GotWasmMessage Decode.Value
    | StartReplay
    | ReplayTick Time.Posix
    | UpdateConfig ConfigField String


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


type EpisodeDoneKind
    = EpisodeComplete String
    | TrainingComplete String


classifyEpisodeDone : String -> EpisodeDoneKind
classifyEpisodeDone content =
    if String.startsWith "Training complete" content then
        TrainingComplete content

    else
        EpisodeComplete content


applyWasmMessage : WasmMessage -> Model -> Model
applyWasmMessage wasmMsg model =
    let
        withHistory =
            { model | receivedMessages = model.receivedMessages ++ [ wasmMsg ] }
    in
    case wasmMsg.msgType of
        "board" ->
            { withHistory | board = parseBoard wasmMsg.content }

        "log" ->
            { withHistory | logs = model.logs ++ [ Log wasmMsg.content ] }

        "error" ->
            { withHistory | logs = model.logs ++ [ Error wasmMsg.content ] }

        "episode_done" ->
            case classifyEpisodeDone wasmMsg.content of
                EpisodeComplete msg ->
                    { withHistory
                        | logs = model.logs ++ [ Log msg ]
                        , replayIndex = Just (List.length withHistory.receivedMessages - 1)
                    }

                TrainingComplete msg ->
                    -- Training is done: log it, flag training as stopped,
                    -- set replay to the last frame, and clear the message buffer
                    -- so the replay starts clean.
                    { withHistory
                        | logs = model.logs ++ [ Log msg ]
                        , training = False
                        , replayIndex = Just (List.length withHistory.receivedMessages - 1)
                        , receivedMessages = []
                    }

        _ ->
            { withHistory | logs = model.logs ++ [ Error ("unknown message type: " ++ wasmMsg.msgType) ] }


update : Msg -> Model -> ( Model, Cmd Msg )
update msg model =
    case msg of
        SendStep value ->
            let
                payload =
                    Encode.object
                        [ ( "type", Encode.string "STEP" )
                        , ( "value", Encode.string value )
                        ]
            in
            ( model, sendToJs payload )

        SendManual ->
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
            ( { model | logs = [ Log "Sent manual command to WASM" ], receivedMessages = [] }, sendToJs payload )

        SendTrain object ->
            let
                payload =
                    Encode.object
                        [ ( "type", Encode.string "TRAIN" )
                        , ( "value", object )
                        ]
            in
            ( { model | logs = [ Log "Sent train command to WASM" ], receivedMessages = [], training = True }, sendToJs payload )

        SendStopTrain ->
            ( { model | logs = model.logs ++ [ Log "Sent stop training command to WASM" ], receivedMessages = [], training = False }, Cmd.none )

        SendAI ->
            let
                payload =
                    Encode.object
                        [ ( "type", Encode.string "AI" )
                        , ( "value", Encode.string "start AI play" )
                        ]
            in
            ( { model | logs = [ Log "Sent AI play command to WASM" ], receivedMessages = [] }, sendToJs payload )

        GotWasmMessage value ->
            case decodeWasmMessage value of
                Ok wasmMsg ->
                    ( applyWasmMessage wasmMsg model, Cmd.none )

                Err _ ->
                    ( { model | logs = model.logs ++ [ Error "Failed to decode WASM message" ] }
                    , Cmd.none
                    )

        UpdateConfig field str ->
            ( { model | config = updateConfig field str model.config }, Cmd.none )

        StartReplay ->
            case model.replayIndex of
                Just _ ->
                    -- Replay already in progress
                    ( model, Cmd.none )

                Nothing ->
                    let
                        lastIndex =
                            List.length model.receivedMessages - 1
                    in
                    ( { model
                        | replayIndex = Just lastIndex
                        , logs = model.logs ++ [ Log "Starting replay.." ]
                      }
                    , Cmd.none
                    )

        ReplayTick _ ->
            case model.replayIndex of
                Nothing ->
                    ( model, Cmd.none )

                Just idx ->
                    let
                        -- receivedMessages is oldest-first; we replay from idx down to 0
                        currentMsg =
                            model.receivedMessages
                                |> List.reverse
                                |> List.drop idx
                                |> List.head
                    in
                    case currentMsg of
                        Nothing ->
                            ( { model | replayIndex = Nothing }, Cmd.none )

                        Just wasmMsg ->
                            let
                                ( updatedModel, nextIndex, cmd ) =
                                    if idx <= 0 then
                                        ( applyWasmMessage wasmMsg { model | receivedMessages = [] }
                                        , Nothing
                                        , sendToJs
                                            (Encode.object
                                                [ ( "type", Encode.string "RESUME_TRAIN" )
                                                , ( "value", Encode.string "continue" )
                                                ]
                                            )
                                        )

                                    else
                                        ( applyWasmMessage wasmMsg model, Just (idx - 1), Cmd.none )
                            in
                            ( { updatedModel | replayIndex = nextIndex }, cmd )


parseBoard : String -> Maybe Board
parseBoard content =
    let
        rows =
            content
                |> String.lines
                |> List.filter (not << String.isEmpty)
                |> List.map parseLine
    in
    if List.isEmpty rows then
        Nothing

    else
        Just { rows = rows }


parseLine : String -> List Cell
parseLine =
    String.toList >> List.map charToCell


charToCell : Char -> Cell
charToCell char =
    case char of
        'W' ->
            Wall

        'S' ->
            Snake

        'H' ->
            Head

        'G' ->
            Green

        'R' ->
            Red

        _ ->
            Empty



-- VIEW


view : Model -> Html Msg
view model =
    div [ class "container" ]
        [ div [ class "game-section", preventDefaultOn "keydown" arrowKeyDecoder ]
            [ h3 [] [ text "Game Board" ]
            , viewBoard model.board
            , viewControls
            ]
        , div [ class "info-section" ]
            [ viewLogs model.logs ]
        , div [ class "input-section" ]
            [ viewAppControl model
            , viewConfig model.config
            ]
        ]


viewBoard : Maybe Board -> Html Msg
viewBoard maybeBoard =
    case maybeBoard of
        Nothing ->
            div [ class "board-placeholder" ] [ text "Waiting for board data..." ]

        Just board ->
            table [ class "game-board" ]
                [ tbody [] (List.map viewRow board.rows) ]


viewRow : List Cell -> Html Msg
viewRow cells =
    tr [] (List.map viewCell cells)


viewCell : Cell -> Html Msg
viewCell cell =
    let
        ( cellClass, cellChar ) =
            case cell of
                Wall ->
                    ( "cell-wall", "W" )

                Empty ->
                    ( "cell-empty", " " )

                Snake ->
                    ( "cell-snake", "S" )

                Head ->
                    ( "cell-head", "H" )

                Green ->
                    ( "cell-green", "G" )

                Red ->
                    ( "cell-red", "R" )
    in
    td [ class ("cell " ++ cellClass) ] [ text cellChar ]


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
    div [ class "control-buttons", preventDefaultOn "keydown" arrowKeyDecoder ]
        [ button [ onClick SendManual ] [ text "Manual play" ]
        , button [ onClick SendAI ] [ text "AI play" ]
        , if model.training then
            button [ onClick SendStopTrain ] [ text "Stop Training" ]

          else
            button [ onClick (SendTrain (encodeConfig model.config)) ] [ text "Train" ]
        ]


viewIntField : String -> ConfigField -> Field Int -> Html Msg
viewIntField labelText field f =
    span [ class "config-field" ]
        [ label [] [ text labelText ]
        , input [ type_ "number", value f.raw, onInput (UpdateConfig field) ] []
        ]


viewFloatField : String -> ConfigField -> Field Float -> Html Msg
viewFloatField labelText field f =
    span [ class "config-field" ]
        [ label [] [ text labelText ]
        , input [ type_ "number", step "0.1", value f.raw, onInput (UpdateConfig field) ] []
        ]


viewConfig : Config -> Html Msg
viewConfig config =
    div [ class "config" ]
        [ h3 [] [ text "Training Configuration" ]
        , div [ class "config-grid" ]
            [ viewIntField "Episodes" Episodes config.episodes
            , viewIntField "Batch Size" BatchSize config.batchSize
            , viewIntField "Max Steps" MaxSteps config.maxSteps
            , viewIntField "Frame Time (ms)" FrameTime config.frameTimeMs
            , viewIntField "Board X" BoardX config.boardX
            , viewIntField "Board Y" BoardY config.boardY
            , viewFloatField "Alpha" Alpha config.alpha
            , viewFloatField "Gamma" Gamma config.gamma
            , viewFloatField "Epsilon" Epsilon config.epsilon
            , viewFloatField "Epsilon Decay" EpsilonDecay config.epsilonDecay
            , viewFloatField "Epsilon Min" EpsilonMin config.epsilonMin
            , viewFloatField "Reward Advance" RewardAdvance config.rewardAdvance
            , viewFloatField "Reward Green" RewardGreen config.rewardGreen
            , viewFloatField "Reward Red" RewardRed config.rewardRed
            , viewFloatField "Reward Death" RewardDeath config.rewardDeath
            ]
        ]


viewLogs : List LogType -> Html Msg
viewLogs logs =
    div [ class "logs-container" ]
        [ h3 [] [ text "Logs" ]
        , div [ class "logs" ]
            (if List.isEmpty logs then
                [ text "No logs yet" ]

             else
                List.map viewLog (List.reverse logs)
            )
        ]


viewLog : LogType -> Html Msg
viewLog entry =
    case entry of
        Log message ->
            div [ class "log-entry" ] [ text message ]

        Error message ->
            div [ class "error-entry" ] [ text message ]


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
        , case model.replayIndex of
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
