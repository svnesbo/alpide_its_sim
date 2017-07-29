/**
 * @file   CTP.hpp
 * @author Simon Voigt Nesbo
 * @date   June 13, 2017
 * @brief  A dummy version of Central Trigger Processor (CTP).
 *         It just adds a 1200ns delay to the trigger to correspond to the
 *         LM trigger delay in the ITS upgrade, there is no selection of
 *         triggers going on.
 */


class CTP  : sc_core::sc_module {
  sc_event E_physics_trigger_in;
  sc_event E_trigger_delayed_out;

private:
  /// Total trigger delay, including wire delay to Readout Unit.
  /// For LM (Level Minus) it is 1200 ns (default)
  unsigned int mTotalTriggerDelay = 1200;

  void triggerInputMethod(void) {
    E_trigger_delayed_out.notify(mTotalTriggerDelay, SC_NS);
  }
}
