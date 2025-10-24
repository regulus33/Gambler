//
// Created by Gambler developers on 20.10.2025.
//

#ifndef GAMBLER_CONTROLS_H
#define GAMBLER_CONTROLS_H

/**
* @class Controls
* @details Most important class in the app as it is the entrypoint for all functionality. Every function is a button basically
*          and all buttons and sliders will be pushing state into these functions.
 */
class Controls {
public:
    Controls() = default;

    /**
    * @brief Sets the shift control state which is a momentary switch
    * @param shift The new shift state
    **/
    void setShift (const bool shift) {
        this->shift = shift;
    }

    /**
    * @brief Gets the shift control state which is a momentary switch
    * @return the current shift state
    **/
    bool getShift() const {
        return this->shift;
    }

    /**
    * @brief Sets the random (random ON|OFF) control state which is a momentary switch
    * @param random the new random val to set
    **/
    void setRandom (bool random) {
        this->random = random;
    }

    /**
    * @brief Gets the random (random ON|OFF) control state which is a momentary switch
    * @param random the current random state
    **/
    bool getRandom() const {
        return this->random;
    }

    // TODO:
    // indexLeft(), indexRight(), lengthMinus(), lengthPlus(), nudgeLeft(), nudgeRight(),

private:
    bool roll = false;
    bool shift = false;
    bool random = false;
};

#endif //GAMBLER_CONTROLS_H
