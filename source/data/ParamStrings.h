//
// Centralized parameter ID and name constants
//
#ifndef GAMBLER_PARAMSTRINGS_H
#define GAMBLER_PARAMSTRINGS_H

struct ParamStrings
{
    // Global parameters
    static constexpr const char* stepLengthId   = "step_length";
    static constexpr const char* stepLengthName = "Step Length";

    static constexpr const char* currentStepId   = "currentStep";
    static constexpr const char* currentStepName = "Current Step";

    static constexpr const char* swingId   = "swing";
    static constexpr const char* swingName = "Swing";
};

#endif // GAMBLER_PARAMSTRINGS_H
