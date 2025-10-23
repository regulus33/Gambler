//
// Created by Gambler developers on 20.10.2025.
//

#ifndef GAMBLER_CONTROLS_H
#define GAMBLER_CONTROLS_H

class Controls
{
public:
    Controls() = default;

    void setShift(bool shift)
    {
        _shift = shift;
    }

    bool getShift() const
    {
        return _shift;
    }

    void setRoll(bool roll)
    {
        _roll = roll;
    }

    bool getRoll() const
    {
        return _roll;
    }

    void setRandom(bool random)
    {
        _random = random;
    }

    bool getRandom() const
    {
        return _random;
    }

    // TODO:
    // indexLeft(), indexRight(), lengthMinus(), lengthPlus(), nudgeLeft(), nudgeRight(),

private:
    bool _roll = false;
    bool _shift = false;
    bool _random = false;
};

#endif //GAMBLER_CONTROLS_H
