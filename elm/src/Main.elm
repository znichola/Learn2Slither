port module Main exposing (main)

import App.Board exposing (..)
import App.Config exposing (..)
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


defaultConfig : Config
defaultConfig =
    { episodes = fieldInt 5000
    , samplePerReplay = fieldInt 100 |> updateFieldHint "Samples before training animation is played."
    , maxSteps = fieldInt 500 |> updateFieldHint "Max steps per episode before stop."
    , frameTimeMs = fieldInt 100 |> updateFieldHint "During playback the delay between frames in ms."
    , boardX = fieldInt 10
    , boardY = fieldInt 10
    , alpha = fieldFloat 0.1 |> updateFieldHint "Learning rate. When lower, it's slower but more stable. (0.0 - 1.0)"
    , gamma = fieldFloat 0.95 |> updateFieldHint "Discount factor for future rewards. When higher, the model prioritises future rewards more, but too high and the snake may loop to stay alive rather than risk seeking food. (0.0 - 1.0)"
    , epsilon = fieldFloat 0.7 |> updateFieldHint "Exploration rate. At 1.0 it will always take random actions. (0.0 - 1.0)"
    , epsilonDecay = fieldFloat 0.995 |> updateFieldHint "Multiplied with epsilon each episode. When lower, exploration drops off faster. Set to 0.0 to remove decay."
    , epsilonMin = fieldFloat 0.0 |> updateFieldHint "Floor for epsilon when using epsilon decay."
    , rewardAdvance = fieldFloat -0.1
    , rewardGreen = fieldFloat 10.0
    , rewardRed = fieldFloat -2.0
    , rewardDeath = fieldFloat -10.0
    }



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

        "batch_done" ->
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
            , viewConfig model.config UpdateConfig
            ]
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
    div [ class "control-buttons", preventDefaultOn "keydown" arrowKeyDecoder ]
        [ button [ onClick SendManual ] [ text "Manual play" ]
        , button [ onClick SendAI ] [ text "AI play" ]
        , if model.training then
            button [ onClick SendStopTrain ] [ text "Stop Training" ]

          else
            button [ onClick (SendTrain (encodeConfig model.config)) ] [ text "Train" ]
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
