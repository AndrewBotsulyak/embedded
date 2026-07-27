#include <led/led.h>

#include <config/config.h>

LedADC::LedADC(uint8_t pin, LedADCOptions options)
  : _pin(pin),
    _mode(LedMode::LDR),
    _options(options) {}

void LedADC::init() {
  ledcSetup(_options.channel, _options.frequency, _options.resolution);
  ledcAttachPin(_pin, _options.channel);
  
}

void LedADC::setMode(LedMode mode) {
  _mode = mode;
}

LedMode LedADC::getMode() const {
  return _mode;
}

void LedADC::handleLDRDuty(uint16_t rawDuty) {
  uint16_t result = 0;

  if (rawDuty > Config::adcDutyTreshold) {
    // increase led brightness in some percents
    uint16_t ledBrightnessIncreaseValue = (rawDuty - Config::adcDutyTreshold) / Config::dutyOneStep;

    result = (ledBrightnessIncreaseValue * Config::ledBrightnessPercentValue) + configuredDutyLight;
  }
  else if (rawDuty > Config::minDutyTreshold) {
    result = Config::minLedBrightness;
  }

  dutyWrite(result);
}

void LedADC::nextMode() {
  switch (_mode) {
    case LedMode::LDR:
      setMode(LedMode::CONSTANT_LIGHT);
      break;
    case LedMode::CONSTANT_LIGHT:
      setMode(LedMode::BLINK);
      break;
    case LedMode::BLINK:
      setMode(LedMode::LDR);
      break;
    default:
      setMode(LedMode::LDR);
      break;
  }
}

void LedADC::handleBlink() {
  ulong now = millis();
  static ulong lastTimeChecked = 0;
  static ulong isOn = true;

  if ((now - lastTimeChecked) >= Config::BLINK_DELAY) {
    ledcWrite(Config::ledADCOptions.channel, isOn ? configuredDutyLight + Config::minLedBrightness : 0);
    lastTimeChecked = now;
    isOn = !isOn;
  }
}

void LedADC::handleBehavior(bool isIncrease) {
  uint32_t minimum = Config::minLedBrightness;
  uint32_t requestedValue = isIncrease ? 
    configuredDutyLight + Config::ledLightStep :
    configuredDutyLight - Config::ledLightStep;
  uint16_t nextLedDuty = requestedValue + Config::minLedBrightness;

  if (nextLedDuty < Config::maxLedBrightness && nextLedDuty > Config::minLedBrightness) {
    configuredDutyLight = requestedValue;
    dutyWrite(nextLedDuty);
  }
}

void LedADC::dutyWrite(uint16_t duty) {
  ledcWrite(_options.channel, duty);
}

uint32_t LedADC::getCurrentDuty() {
  return ledcRead(_options.channel);
}
