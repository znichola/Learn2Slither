module App.Config exposing (..)

import Html exposing (Html, div, h3, input, label, option, select, span, text)
import Html.Attributes exposing (attribute, class, selected, step, type_, value)
import Html.Events exposing (onInput)
import Json.Decode as Decode exposing (Decoder)
import Json.Encode as Encode



-- MODEL


type ConfigField
    = Episodes
    | SamplePerReplay
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
    | StateFn


type alias Config =
    { episodes : Field Int
    , samplePerReplay : Field Int
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
    , stateFn : StateType
    }


type alias Field a =
    { raw : String
    , parsed : Maybe a
    , default : a
    , hint : Maybe String
    }


type StateType
    = Full
    | FirstNonEmpty
    | FirstAndNextNonEmpty



-- FIELD


fieldInt : Int -> Field Int
fieldInt n =
    { raw = String.fromInt n, parsed = Just n, default = n, hint = Nothing }


fieldFloat : Float -> Field Float
fieldFloat n =
    { raw = String.fromFloat n, parsed = Just n, default = n, hint = Nothing }


updateFieldInt : String -> Field Int -> Field Int
updateFieldInt str f =
    { f | raw = str, parsed = String.toInt str }


updateFieldFloat : String -> Field Float -> Field Float
updateFieldFloat str f =
    { f | raw = str, parsed = String.toFloat str }


updateFieldHint : String -> Field a -> Field a
updateFieldHint hint f =
    { f | hint = Just hint }


getField : Field a -> a
getField field =
    Maybe.withDefault field.default field.parsed


stateToString : StateType -> String
stateToString s =
    case s of
        Full ->
            "FULL"

        FirstNonEmpty ->
            "FIRST_NON_EMPTY"

        FirstAndNextNonEmpty ->
            "FIRST_AND_NEXT_NON_EMPTY"


stringToState : String -> StateType
stringToState str =
    case str of
        "FULL" ->
            Full

        "FIRST_NON_EMPTY" ->
            FirstNonEmpty

        "FIRST_AND_NEXT_NON_EMPTY" ->
            FirstAndNextNonEmpty

        _ ->
            Full



-- VIEW


viewTitle : String -> Field a -> Html msg
viewTitle labelText f =
    case f.hint of
        Just hint ->
            label [ attribute "aria-label" hint, class "config-hint" ] [ text labelText ]

        Nothing ->
            label [] [ text labelText ]


viewIntField : String -> ConfigField -> Field Int -> (ConfigField -> String -> msg) -> Html msg
viewIntField labelText field f toMsg =
    span [ class "config-field" ]
        [ viewTitle labelText f
        , input [ type_ "number", value f.raw, onInput (toMsg field) ] []
        ]


viewFloatField : String -> ConfigField -> Field Float -> (ConfigField -> String -> msg) -> Html msg
viewFloatField labelText field f toMsg =
    span [ class "config-field" ]
        [ viewTitle labelText f
        , input [ type_ "number", step "0.1", value f.raw, onInput (toMsg field) ] []
        ]


viewEnumField :
    String
    -> ConfigField
    -> StateType
    -> (ConfigField -> String -> msg)
    -> Html msg
viewEnumField labelText field current toMsg =
    let
        options =
            [ Full, FirstNonEmpty, FirstAndNextNonEmpty ]
    in
    span [ class "config-field" ]
        [ label [] [ text labelText ]
        , select [ onInput (toMsg field) ]
            (List.map
                (\opt ->
                    option
                        [ value (stateToString opt)
                        , selected (opt == current)
                        ]
                        [ text (stateToString opt) ]
                )
                options
            )
        ]


viewConfig : Config -> (ConfigField -> String -> msg) -> Html msg
viewConfig config toMsg =
    div [ class "config" ]
        [ h3 [] [ text "Training Configuration" ]
        , div [ class "config-grid" ]
            [ viewIntField "Episodes" Episodes config.episodes toMsg
            , viewIntField "Sample per Replay" SamplePerReplay config.samplePerReplay toMsg
            , viewIntField "Max Steps" MaxSteps config.maxSteps toMsg
            , viewIntField "Frame Time (ms)" FrameTime config.frameTimeMs toMsg
            , viewIntField "Board X" BoardX config.boardX toMsg
            , viewIntField "Board Y" BoardY config.boardY toMsg
            , viewFloatField "Alpha" Alpha config.alpha toMsg
            , viewFloatField "Gamma" Gamma config.gamma toMsg
            , viewFloatField "Epsilon" Epsilon config.epsilon toMsg
            , viewFloatField "Epsilon Decay" EpsilonDecay config.epsilonDecay toMsg
            , viewFloatField "Epsilon Min" EpsilonMin config.epsilonMin toMsg
            , viewFloatField "Reward Advance" RewardAdvance config.rewardAdvance toMsg
            , viewFloatField "Reward Green" RewardGreen config.rewardGreen toMsg
            , viewFloatField "Reward Red" RewardRed config.rewardRed toMsg
            , viewFloatField "Reward Death" RewardDeath config.rewardDeath toMsg
            , viewEnumField "State Representation" StateFn config.stateFn toMsg
            ]
        ]



-- UPDATE


updateConfig : ConfigField -> String -> Config -> Config
updateConfig field str config =
    case field of
        Episodes ->
            { config | episodes = updateFieldInt str config.episodes }

        SamplePerReplay ->
            { config | samplePerReplay = updateFieldInt str config.samplePerReplay }

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

        StateFn ->
            { config | stateFn = stringToState str }



-- ENCODE / DEcODE


encodeConfig : Config -> Encode.Value
encodeConfig config =
    Encode.object
        [ ( "EPISODES", Encode.int (getField config.episodes) )
        , ( "SAMPLE_PER_REPLAY", Encode.int (getField config.samplePerReplay) )
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
        , ( "statefn", Encode.string (stateToString config.stateFn) )
        ]


required : String -> Decoder a -> Decoder (a -> b) -> Decoder b
required fieldName decoder decoderFunc =
    Decode.map2
        (\func value -> func value)
        decoderFunc
        (Decode.field fieldName decoder)


optional : String -> Decoder a -> a -> Decoder (a -> b) -> Decoder b
optional fieldName decoder defaultValue decoderFunc =
    Decode.map2
        (\func value -> func value)
        decoderFunc
        (Decode.oneOf
            [ Decode.field fieldName decoder
            , Decode.succeed defaultValue
            ]
        )


hardcoded : a -> Decoder a
hardcoded =
    Decode.succeed


configDecoder : Decoder Config
configDecoder =
    Decode.succeed Config
        |> required "EPISODES" (Decode.int |> Decode.map fieldInt)
        |> required "SAMPLE_PER_REPLAY" (Decode.int |> Decode.map fieldInt)
        |> required "MAX_STEPS" (Decode.int |> Decode.map fieldInt)
        |> required "frame_time_ms" (Decode.int |> Decode.map fieldInt)
        |> required "board_x" (Decode.int |> Decode.map fieldInt)
        |> required "board_y" (Decode.int |> Decode.map fieldInt)
        |> required "alpha" (Decode.float |> Decode.map fieldFloat)
        |> required "gamma" (Decode.float |> Decode.map fieldFloat)
        |> required "epsilon" (Decode.float |> Decode.map fieldFloat)
        |> required "epsilon_decay" (Decode.float |> Decode.map fieldFloat)
        |> required "epsilon_min" (Decode.float |> Decode.map fieldFloat)
        |> required "reward_advance" (Decode.float |> Decode.map fieldFloat)
        |> required "reward_green" (Decode.float |> Decode.map fieldFloat)
        |> required "reward_red" (Decode.float |> Decode.map fieldFloat)
        |> required "reward_death" (Decode.float |> Decode.map fieldFloat)
        |> required "statefn" (Decode.string |> Decode.map stringToState)


decodeConfig : String -> Result Decode.Error Config
decodeConfig =
    Decode.decodeString configDecoder
