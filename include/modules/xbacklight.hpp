#pragma once

#include "components/config.hpp"
#include "components/x11/connection.hpp"
#include "components/x11/randr.hpp"
#include "components/x11/xutils.hpp"
#include "config.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta.hpp"
#include "utils/throttle.hpp"

LEMONBUDDY_NS

namespace modules {
  /**
   * Backlight module built using the RandR X extension.
   *
   * This is built as a replacement for the old backlight
   * module that was set up using with inotify watches listening
   * for changes to the raw file handle.
   *
   * This module is alot faster, it's more responsive and it will
   * be dormant until new values are reported. Inotify watches
   * are a bit random when it comes to proc-/sysfs.
   *
   * TODO: Implement backlight configuring using scroll events
   */
  class xbacklight_module : public static_module<xbacklight_module>,
                            public xpp::event::sink<evt::randr_notify> {
   public:
    using static_module::static_module;

    /**
     * Bootstrap the module by grabbing all required components
     */
    void setup() {
      auto output = m_conf.get<string>(name(), "output", m_bar.monitor->name);

      // Grab a list of all outputs and try to find the one defined in the config
      for (auto&& mon : randr_util::get_monitors(m_connection, m_connection.root())) {
        if (mon->name == output) {
          m_output.swap(mon);
          break;
        }
      }

      // If we didn't get a match we stop the module
      if (!m_output) {
        throw module_error("No matching output found for \"" + output + "\", stopping module...");
      }

      // Query randr for the backlight max and min value
      try {
        auto& backlight = m_output->backlight;
        randr_util::get_backlight_range(m_connection, m_output, backlight);
        randr_util::get_backlight_value(m_connection, m_output, backlight);
      } catch (const std::exception& err) {
        throw module_error("No backlight data found for \"" + output + "\", stopping module...");
      }

      // Connect with the event registry and tell randr that we
      // want to get notified when an output property gets modified
      m_connection.attach_sink(this, 1);
      m_connection.select_input_checked(
          m_connection.screen()->root, XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY);

      // Create a throttle so that we limit the amount of events
      // to handle since randr can burst out quite a few
      // We will allow 1 event per 60 ms. The updates still look smooth
      // using this setting which is important.
      m_throttler = throttle_util::make_throttler(1, 60ms);

      // Add formats and elements
      m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_BAR, TAG_RAMP});

      if (m_formatter->has(TAG_LABEL))
        m_label = load_optional_label(m_conf, name(), TAG_LABEL, "%percentage%");
      if (m_formatter->has(TAG_BAR))
        m_progressbar = load_progressbar(m_bar, m_conf, name(), TAG_BAR);
      if (m_formatter->has(TAG_RAMP))
        m_ramp = load_ramp(m_conf, name(), TAG_RAMP);

      // Trigger the initial draw event
      update();
    }

    /**
     * Handler for XCB_RANDR_NOTIFY events
     */
    void handle(const evt::randr_notify& evt) {
      if (evt->subCode != XCB_RANDR_NOTIFY_OUTPUT_PROPERTY)
        return;
      else if (evt->u.op.output != m_output->randr_output)
        return;
      else if (evt->u.op.atom == Backlight)
        update();
      else if (evt->u.op.atom == BACKLIGHT)
        update();
    }

    /**
     * Query the RandR extension for the new values
     */
    void update() {
      // Test if we are allowed to handle the event
      if (!m_throttler->passthrough(throttle_util::strategy::try_once_or_leave_yolo{}))
        return;

      // Query for the new backlight value
      auto& bl = m_output->backlight;
      randr_util::get_backlight_value(m_connection, m_output, bl);
      m_percentage = math_util::percentage(bl.val, bl.min, bl.max);

      // Update label tokens
      if (m_label) {
        m_label->reset_tokens();
        m_label->replace_token("%percentage%", to_string(m_percentage) + "%");
      }

      // Emit a broadcast notification so that
      // the new data will be drawn to the bar
      broadcast();
    }

    /**
     * Output content as defined in the config
     */
    bool build(builder* builder, string tag) const {
      if (tag == TAG_BAR)
        builder->node(m_progressbar->output(m_percentage));
      else if (tag == TAG_RAMP)
        builder->node(m_ramp->get_by_percentage(m_percentage));
      else if (tag == TAG_LABEL)
        builder->node(m_label);
      else
        return false;
      return true;
    }

   private:
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_BAR = "<bar>";
    static constexpr auto TAG_RAMP = "<ramp>";

    throttle_util::throttle_t m_throttler;
    connection& m_connection{configure_connection().create<connection&>()};
    monitor_t m_output;

    ramp_t m_ramp;
    label_t m_label;
    progressbar_t m_progressbar;

    int m_percentage = 0;
  };
}

LEMONBUDDY_NS_END
