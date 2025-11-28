port module Main exposing (main)

import Browser
import Html exposing (Html, button, div, text)
import Html.Events exposing (onClick)
import Json.Decode as Decode
import Json.Encode as Encode


-- MODEL

type alias Model =
    { log : List String }


init : () -> ( Model, Cmd Msg )
init _ =
    ( { log = [ "Elm loaded. Waiting for WASM…" ] }
    , Cmd.none
    )



-- PORTS

-- Elm → JS
port sendToJs : Encode.Value -> Cmd msg

-- JS → Elm
port receiveFromJs : (Decode.Value -> msg) -> Sub msg



-- UPDATE

type Msg
    = SendStep
    | GotJsMessage Decode.Value


update : Msg -> Model -> ( Model, Cmd Msg )
update msg model =
    case msg of
        SendStep ->
            let
                payload =
                    Encode.object
                        [ ( "type", Encode.string "step" )
                        , ( "value", Encode.int 1 )  -- example action
                        ]
            in
            ( model, sendToJs payload )

        GotJsMessage value ->
            let
                decoded =
                    case Decode.decodeValue (Decode.field "text" Decode.string) value of
                        Ok t ->
                            t

                        Err _ ->
                            "JS sent non-string payload"
            in
            ( { model | log = model.log ++ [ decoded ] }
            , Cmd.none
            )



-- VIEW

view : Model -> Html Msg
view model =
    div []
        [ button [ onClick SendStep ] [ text "Send step → WASM" ]
        , div [] (List.map (\line -> div [] [ text line ]) model.log)
        ]


-- SUBSCRIPTIONS

subscriptions : Model -> Sub Msg
subscriptions _ =
    receiveFromJs GotJsMessage


-- MAIN

main : Program () Model Msg
main =
    Browser.element
        { init = init
        , view = view
        , update = update
        , subscriptions = subscriptions
        }

