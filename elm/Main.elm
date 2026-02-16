port module Main exposing (main)

import Browser
import Html exposing (Html, button, input, div, h3, table, tbody, td, text, tr)
import Html.Attributes exposing (class, style)
import Html.Events exposing (onClick)
import Json.Decode as Decode
import Json.Encode as Encode



-- MODEL


type alias Model =
    { board : Maybe Board
    , logs : List String
    , errors : List String
    }


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
      , errors = []
      }
    , Cmd.none
    )



-- PORTS


port sendToJs : Encode.Value -> Cmd msg


port receiveFromJs : (Decode.Value -> msg) -> Sub msg



-- UPDATE


type Msg
    = SendStep String
    | GotWasmMessage Decode.Value


update : Msg -> Model -> ( Model, Cmd Msg )
update msg model =
    case msg of
        SendStep value ->
            let
                payload =
                    Encode.object
                        [ ( "type", Encode.string "step" )
                        , ( "value", Encode.string value )
                        ]
            in
            ( model, sendToJs payload )

        GotWasmMessage value ->
            case decodeWasmMessage value of
                Ok wasmMsg ->
                    case wasmMsg.msgType of
                        "board" ->
                            ( { model | board = parseBoard wasmMsg.content }
                            , Cmd.none
                            )

                        "log" ->
                            ( { model | logs = model.logs ++ [ wasmMsg.content ] }
                            , Cmd.none
                            )

                        "error" ->
                            ( { model | errors = model.errors ++ [ wasmMsg.content ] }
                            , Cmd.none
                            )

                        _ ->
                            ( model, Cmd.none )

                Err _ ->
                    ( { model | errors = model.errors ++ [ "Failed to decode WASM message" ] }
                    , Cmd.none
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
            , viewErrors model.errors
            ]
        , div [ class "input-section"]
            [ viewConfig ]
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


viewControles: Html Msg
viewControles= div [class "controles"] [
        button [ onClick (SendStep "UP")]  [text "up"]
        , button [ onClick(SendStep "DOWN")]  [text "down"]
        , button [ onClick(SendStep "LEFT")]  [text "left"]
        , button [ onClick(SendStep "RIGHT")]  [text "right"]
    ]

viewConfig: Html Msg
viewConfig = div [] 
    [ div [] [text "foobar"]
    , input [] [text "text"]
    ]

viewLogs : List String -> Html Msg
viewLogs logs =
    div [ class "logs-container" ]
        [ h3 [] [ text "Logs" ]
        , div [ class "logs" ]
            (if List.isEmpty logs then
                [ text "No logs yet" ]

             else
                List.map (\log -> div [ class "log-entry" ] [ text log ]) (List.reverse logs |> List.take 10)
            )
        ]


viewErrors : List String -> Html Msg
viewErrors errors =
    if List.isEmpty errors then
        text ""

    else
        div [ class "errors-container" ]
            [ h3 [] [ text "Errors" ]
            , div [ class "errors" ]
                (List.map (\err -> div [ class "error-entry" ] [ text err ]) (List.reverse errors |> List.take 5))
            ]



-- SUBSCRIPTIONS


subscriptions : Model -> Sub Msg
subscriptions _ =
    receiveFromJs GotWasmMessage



-- MAIN


main : Program () Model Msg
main =
    Browser.element
        { init = init
        , view = view
        , update = update
        , subscriptions = subscriptions
        }
