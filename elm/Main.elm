port module Main exposing (main)

import Browser
import Browser.Events
import Html exposing (Html, button, div, h3, input, label, span, table, tbody, td, text, tr)
import Html.Attributes exposing (class, placeholder, step, style, type_, value)
import Html.Events exposing (onClick, onInput)
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
      }
    , Cmd.none
    )


type alias Config =
    { episodes : Int
    , batchSize : Int
    , maxSteps : Int
    , frame_time_ms : Int
    , boardX : Int
    , boardY : Int
    , alpha : Float
    , gamma : Float
    , epsilon : Float
    , epsilonDecay : Float
    , epsilonMin : Float
    , rewardAdvance : Float
    , rewardGreen : Float
    , rewardRed : Float
    , rewardDeath : Float
    }


defaultConfig : Config
defaultConfig =
    { episodes = 10
    , batchSize = 1
    , maxSteps = 100
    , frame_time_ms = 100
    , boardX = 10
    , boardY = 10
    , alpha = 0.4
    , gamma = 0.9
    , epsilon = 0.1
    , epsilonDecay = 0.995
    , epsilonMin = 0.01
    , rewardAdvance = 0.0
    , rewardGreen = 10000.0
    , rewardRed = 1.0
    , rewardDeath = -10.0
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



-- PORTS


port sendToJs : Encode.Value -> Cmd msg


port receiveFromJs : (Decode.Value -> msg) -> Sub msg



-- UPDATE


type Msg
    = SendStep String
    | SendStart
    | SendTrain Encode.Value
    | GotWasmMessage Decode.Value
    | StartReplay
    | ReplayTick Time.Posix
    | UpdateConfig ConfigField String


applyWasmMessage : WasmMessage -> Model -> Model
applyWasmMessage wasmMsg model =
    model
        |> addToHistory wasmMsg
        |> applyContent wasmMsg


addToHistory : WasmMessage -> Model -> Model
addToHistory wasmMsg model =
    { model
        | receivedMessages =
            model.receivedMessages ++ [ wasmMsg ]
    }


applyContent : WasmMessage -> Model -> Model
applyContent wasmMsg model =
    case wasmMsg.msgType of
        "board" ->
            { model | board = parseBoard wasmMsg.content }

        "log" ->
            { model | logs = model.logs ++ [ Log wasmMsg.content ] }

        "error" ->
            { model | logs = model.logs ++ [ Error wasmMsg.content ] }

        "episode_done" ->
            { model
                | logs = model.logs ++ [ Log wasmMsg.content ]
                , replayIndex = Just (List.length model.receivedMessages - 1)
            }

        _ ->
            { model | logs = model.logs ++ [ Error ("unknown message type " ++ wasmMsg.content) ] }


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

        SendStart ->
            let
                payload =
                    Encode.object
                        [ ( "type", Encode.string "START" )
                        , ( "value", Encode.string "start playing" )
                        ]
            in
            ( { model | logs = [ Log "Sent start command to WASM" ], receivedMessages = [] }, sendToJs payload )

        SendTrain object ->
            let
                payload =
                    Encode.object
                        [ ( "type", Encode.string "TRAIN" )
                        , ( "value", object )
                        ]
            in
            ( { model | logs = [ Log "Sent train command to WASM" ], receivedMessages = [] }, sendToJs payload )

        GotWasmMessage value ->
            case decodeWasmMessage value of
                Ok wasmMsg ->
                    ( applyWasmMessage wasmMsg model
                    , Cmd.none
                    )

                Err _ ->
                    ( { model | logs = model.logs ++ [ Error "Failed to decode WASM message" ] }
                    , Cmd.none
                    )

        UpdateConfig field str ->
            ( { model
                | config = updateConfig field str model.config
              }
            , Cmd.none
            )

        StartReplay ->
            case model.replayIndex of
                Just _ ->
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
                        msgFromEnd =
                            model.receivedMessages
                                |> List.reverse
                                |> List.drop idx
                                |> List.head
                    in
                    case msgFromEnd of
                        Nothing ->
                            ( { model | replayIndex = Nothing }
                            , Cmd.none
                            )

                        Just wasmMsg ->
                            let
                                ( updatedModel, nextIndex, cmd ) =
                                    if idx <= 0 then
                                        ( applyContent wasmMsg { model | receivedMessages = [] }
                                        , Nothing
                                        , sendToJs
                                            (Encode.object
                                                [ ( "type", Encode.string "RESUME_TRAIN" )
                                                , ( "value", Encode.string "continue" )
                                                ]
                                            )
                                        )

                                    else
                                        ( applyContent wasmMsg model, Just (idx - 1), Cmd.none )
                            in
                            ( { updatedModel | replayIndex = nextIndex }
                            , cmd
                            )


type alias WasmMessage =
    { msgType : String
    , source : String
    , content : String
    }


decodeWasmMessage : Decode.Value -> Result Decode.Error WasmMessage
decodeWasmMessage value =
    Decode.decodeValue
        (Decode.map3 WasmMessage
            (Decode.field "type" Decode.string)
            (Decode.field "source" Decode.string)
            (Decode.field "content" Decode.string)
        )
        value


parseBoard : String -> Maybe Board
parseBoard content =
    let
        lines =
            String.lines content
                |> List.filter (not << String.isEmpty)

        rows =
            List.map parseLine lines
    in
    if List.isEmpty rows then
        Nothing

    else
        Just { rows = rows }


parseLine : String -> List Cell
parseLine line =
    String.toList line
        |> List.map charToCell


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


updateConfig : ConfigField -> String -> Config -> Config
updateConfig field value config =
    case field of
        Episodes ->
            { config | episodes = parseInt value config.episodes }

        BatchSize ->
            { config | batchSize = parseInt value config.batchSize }

        MaxSteps ->
            { config | maxSteps = parseInt value config.maxSteps }

        FrameTime ->
            { config | frame_time_ms = parseInt value config.frame_time_ms }

        BoardX ->
            { config | boardX = parseInt value config.boardX }

        BoardY ->
            { config | boardY = parseInt value config.boardY }

        Alpha ->
            { config | alpha = parseFloat value config.alpha }

        Gamma ->
            { config | gamma = parseFloat value config.gamma }

        Epsilon ->
            { config | epsilon = parseFloat value config.epsilon }

        EpsilonDecay ->
            { config | epsilonDecay = parseFloat value config.epsilonDecay }

        EpsilonMin ->
            { config | epsilonMin = parseFloat value config.epsilonMin }

        RewardAdvance ->
            { config | rewardAdvance = parseFloat value config.rewardAdvance }

        RewardGreen ->
            { config | rewardGreen = parseFloat value config.rewardGreen }

        RewardRed ->
            { config | rewardRed = parseFloat value config.rewardRed }

        RewardDeath ->
            { config | rewardDeath = parseFloat value config.rewardDeath }



-- VIEW


view : Model -> Html Msg
view model =
    div [ class "container" ]
        [ div [ class "game-section" ]
            [ h3 [] [ text "Game Board" ]
            , viewBoard model.board
            , viewControles
            ]
        , div [ class "info-section" ]
            [ viewLogs model.logs
            ]
        , div [ class "input-section" ]
            [ viewButton model
            , viewConfig model.config
            ]
        ]


viewBoard : Maybe Board -> Html Msg
viewBoard maybeBoard =
    case maybeBoard of
        Nothing ->
            div [ class "board-placeholder" ]
                [ text "Waiting for board data..." ]

        Just board ->
            table [ class "game-board" ]
                [ tbody []
                    (List.map viewRow board.rows)
                ]


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
    td [ class ("cell " ++ cellClass) ]
        [ text cellChar ]


viewControles : Html Msg
viewControles =
    div [ class "controles" ]
        [ button [ onClick (SendStep "UP") ] [ text "up" ]
        , button [ onClick (SendStep "DOWN") ] [ text "down" ]
        , button [ onClick (SendStep "LEFT") ] [ text "left" ]
        , button [ onClick (SendStep "RIGHT") ] [ text "right" ]
        ]


viewField : String -> Html Msg -> Html Msg
viewField labelText inputElement =
    span [ class "config-field" ]
        [ label []
            [ text labelText ]
        , inputElement
        ]


viewIntField : String -> ConfigField -> Int -> Html Msg
viewIntField labelText field current =
    viewField labelText <|
        input
            [ type_ "number"
            , value (String.fromInt current)
            , onInput (UpdateConfig field)
            ]
            []


viewFloatField : String -> ConfigField -> Float -> Html Msg
viewFloatField labelText field current =
    viewField labelText <|
        input
            [ type_ "number"
            , step "0.01"
            , value (String.fromFloat current)
            , onInput (UpdateConfig field)
            ]
            []


viewConfig : Config -> Html Msg
viewConfig config =
    div [ class "config" ]
        [ h3 [] [ text "Training Configuration" ]
        , div
            [ class "config-grid" ]
            [ viewIntField "Episodes" Episodes config.episodes
            , viewIntField "Batch Size" BatchSize config.batchSize
            , viewIntField "Max Steps" MaxSteps config.maxSteps
            , viewIntField "Frame Time (ms)" FrameTime config.frame_time_ms
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


encodeConfig : Config -> Encode.Value
encodeConfig config =
    Encode.object
        [ ( "EPISODES", Encode.int config.episodes )
        , ( "BATCH_SIZE", Encode.int config.batchSize )
        , ( "MAX_STEPS", Encode.int config.maxSteps )
        , ( "frame_time_ms", Encode.int config.frame_time_ms )
        , ( "board_x", Encode.int config.boardX )
        , ( "board_y", Encode.int config.boardY )
        , ( "alpha", Encode.float config.alpha )
        , ( "gamma", Encode.float config.gamma )
        , ( "epsilon", Encode.float config.epsilon )
        , ( "epsilon_decay", Encode.float config.epsilonDecay )
        , ( "epsilon_min", Encode.float config.epsilonMin )
        , ( "reward_advance", Encode.float config.rewardAdvance )
        , ( "reward_green", Encode.float config.rewardGreen )
        , ( "reward_red", Encode.float config.rewardRed )
        , ( "reward_death", Encode.float config.rewardDeath )
        ]


viewButton : Model -> Html Msg
viewButton model =
    div [ class "control-buttons" ]
        [ button [ onClick StartReplay ] [ text "Replay" ]
        , button [ onClick SendStart ] [ text "Start" ]
        , button [ onClick (SendTrain (encodeConfig model.config)) ] [ text "Train" ]
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
            div [ class "log-entry" ]
                [ text message ]

        Error message ->
            div [ class "error-entry" ]
                [ text message ]



-- HELPERS


parseInt : String -> Int -> Int
parseInt str fallback =
    case String.toInt str of
        Just n ->
            n

        Nothing ->
            fallback


parseFloat : String -> Float -> Float
parseFloat str fallback =
    case String.toFloat str of
        Just n ->
            n

        Nothing ->
            fallback


keyToStepMsg : String -> Maybe Msg
keyToStepMsg key =
    case key of
        "ArrowUp" ->
            Just (SendStep "UP")

        "ArrowDown" ->
            Just (SendStep "DOWN")

        "ArrowLeft" ->
            Just (SendStep "LEFT")

        "ArrowRight" ->
            Just (SendStep "RIGHT")

        _ ->
            Nothing


arrowKeyDecoder : Decode.Decoder Msg
arrowKeyDecoder =
    Decode.field "key" Decode.string
        |> Decode.andThen
            (\key ->
                case keyToStepMsg key of
                    Just msg ->
                        Decode.succeed msg

                    Nothing ->
                        Decode.fail "Not an arrow key"
            )



-- SUBSCRIPTIONS


subscriptions : Model -> Sub Msg
subscriptions model =
    Sub.batch
        [ receiveFromJs GotWasmMessage
        , case model.replayIndex of
            Just _ ->
                Time.every (toFloat model.config.frame_time_ms) ReplayTick

            Nothing ->
                Sub.none
        , Browser.Events.onKeyDown (Decode.map identity arrowKeyDecoder)
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
