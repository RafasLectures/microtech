#include "ShiftRegister.hpp"

namespace Microtech {

void ShiftRegisterBase::setMode(const Mode mode) const noexcept {
    switch(mode){
    case ShiftRegisterBase::Mode::PAUSE:
        s0.setState(IOState::LOW);
        s1.setState(IOState::LOW);
        break;
    case ShiftRegisterBase::Mode::SHIFT_RIGHT:
        s0.setState(IOState::HIGH);
        s1.setState(IOState::LOW);
        break;
    case ShiftRegisterBase::Mode::SHIFT_LEFT:
        s0.setState(IOState::LOW);
        s1.setState(IOState::HIGH);
        break;
    case ShiftRegisterBase::Mode::MIRROR_PARALLEL:
        s0.setState(IOState::HIGH);
        s1.setState(IOState::HIGH);
        break;
    };
}

void ShiftRegisterBase::init() const noexcept {
    s0.init();
    s1.init();
    s0.setState(IOState::LOW);
    s1.setState(IOState::LOW);

    if(shiftRight != nullptr) {
        shiftRight->init();
        shiftRight->setState(IOState::LOW);
    }

    if (inputQD != nullptr) {
        inputQD->init();
    }
}
void ShiftRegisterBase::outputAStateRightShift(const IOState state) const noexcept {
    if(shiftRight != nullptr) {
        shiftRight->setState(state);
    }
}

IOState ShiftRegisterBase::getInputQDState() const noexcept {
    if (inputQD != nullptr) {
        return inputQD->getState();
    }
    return IOState::LOW;
}

void ShiftRegisterController::init() const noexcept {
    clock.init();
    clear.init();
    clock.setState(IOState::LOW);
    clear.setState(IOState::LOW);
}

void ShiftRegisterController::start() const noexcept {
    clear.setState(IOState::HIGH);
}

void ShiftRegisterController::stop() const noexcept {
    clear.setState(IOState::LOW);
}

void ShiftRegisterController::reset() const noexcept {
    clear.setState(IOState::LOW);
    clear.setState(IOState::HIGH);
}

void ShiftRegisterController::clockOneCycle() const noexcept {
    clock.setState(IOState::HIGH);
    // According to datasheet it takes approximately 6ns for the clock to stabilize.
    // In theory there is no need for a delay
    //__delay_cycles(12);
    clock.setState(IOState::LOW);
}
}
