module App.Board exposing (..)

import Html exposing (Html, div, table, tbody, td, text, tr)
import Html.Attributes exposing (class)



-- MODEL


type alias Board =
    { rows : List (List Cell) }


type Cell
    = Wall
      -- | Empty
    | Value Char
    | Snake
    | Head
    | Green
    | Red



-- VIEW


viewBoard : Maybe Board -> Html msg
viewBoard maybeBoard =
    case maybeBoard of
        Nothing ->
            div [ class "board-placeholder" ] [ text "Waiting for board data..." ]

        Just board ->
            table [ class "game-board" ]
                [ tbody [] (List.map viewRow board.rows) ]


viewRow : List Cell -> Html msg
viewRow cells =
    tr [] (List.map viewCell cells)


viewCell : Cell -> Html msg
viewCell cell =
    let
        ( cellClass, cellChar ) =
            case cell of
                Wall ->
                    ( "cell-wall", "W" )

                Value s ->
                    ( "cell-empty", String.fromChar s )

                -- Empty ->
                --     ( "cell-empty", "0" )
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



-- PARSING


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

        s ->
            Value s
