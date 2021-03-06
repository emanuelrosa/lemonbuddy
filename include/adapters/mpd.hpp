#pragma once

#include <mpd/client.h>
#include <stdlib.h>
#include <chrono>
#include <memory>
#include <string>

#include "common.hpp"
#include "components/logger.hpp"
#include "utils/math.hpp"

LEMONBUDDY_NS

namespace mpd {
  DEFINE_ERROR(mpd_exception);
  DEFINE_CHILD_ERROR(client_error, mpd_exception);
  DEFINE_CHILD_ERROR(server_error, mpd_exception);

  // type details {{{

  namespace details {
    struct mpd_connection_deleter {
      void operator()(mpd_connection* conn) {
        if (conn != nullptr)
          mpd_connection_free(conn);
      }
    };

    struct mpd_status_deleter {
      void operator()(mpd_status* status) {
        mpd_status_free(status);
      }
    };

    struct mpd_song_deleter {
      void operator()(mpd_song* song) {
        mpd_song_free(song);
      }
    };

    using mpd_connection_t = unique_ptr<mpd_connection, mpd_connection_deleter>;
    using mpd_status_t = unique_ptr<mpd_status, mpd_status_deleter>;
    using mpd_song_t = unique_ptr<mpd_song, mpd_song_deleter>;
  }

  inline void check_connection(mpd_connection* conn) {
    if (conn == nullptr)
      throw client_error("Not connected to MPD server", MPD_ERROR_STATE);
  }

  inline void check_errors(mpd_connection* conn) {
    mpd_error code = mpd_connection_get_error(conn);

    if (code == MPD_ERROR_SUCCESS)
      return;

    auto msg = mpd_connection_get_error_message(conn);

    if (code == MPD_ERROR_SERVER) {
      mpd_connection_clear_error(conn);
      throw server_error(msg, mpd_connection_get_server_error(conn));
    } else {
      mpd_connection_clear_error(conn);
      throw client_error(msg, code);
    }
  }

  enum class mpdstate {
    UNKNOWN = 1 << 0,
    STOPPED = 1 << 1,
    PLAYING = 1 << 2,
    PAUSED = 1 << 4,
  };

  // }}}
  // class: mpdsong {{{

  class mpdsong {
   public:
    explicit mpdsong(details::mpd_song_t&& song) : m_song(forward<decltype(song)>(song)) {}

    operator bool() {
      return m_song.get() != nullptr;
    }

    string get_artist() {
      assert(m_song);
      auto tag = mpd_song_get_tag(m_song.get(), MPD_TAG_ARTIST, 0);
      if (tag == nullptr)
        return "";
      return string{tag};
    }

    string get_album() {
      assert(m_song);
      auto tag = mpd_song_get_tag(m_song.get(), MPD_TAG_ALBUM, 0);
      if (tag == nullptr)
        return "";
      return string{tag};
    }

    string get_title() {
      assert(m_song);
      auto tag = mpd_song_get_tag(m_song.get(), MPD_TAG_TITLE, 0);
      if (tag == nullptr)
        return "";
      return string{tag};
    }

    unsigned get_duration() {
      assert(m_song);
      return mpd_song_get_duration(m_song.get());
    }

   private:
    details::mpd_song_t m_song;
  };

  // }}}
  // class: mpdconnection {{{

  class mpdstatus;
  class mpdconnection {
   public:
    explicit mpdconnection(const logger& logger, string host, unsigned int port = 6600, string password = "", unsigned int timeout = 15)
      : m_log(logger), m_host(host), m_port(port), m_password(password), m_timeout(timeout) {}

    void connect() {
      try {
        m_log.trace("mpdconnection.connect: %s, %i, \"%s\", timeout: %i", m_host, m_port, m_password, m_timeout);
        m_connection.reset(mpd_connection_new(m_host.c_str(), m_port, m_timeout * 1000));
        check_errors(m_connection.get());

        if (!m_password.empty()) {
          noidle();
          assert(!m_listactive);
          mpd_run_password(m_connection.get(), m_password.c_str());
          check_errors(m_connection.get());
        }

        m_fd = mpd_connection_get_fd(m_connection.get());
        check_errors(m_connection.get());
      } catch (const client_error& e) {
        disconnect();
        throw e;
      }
    }

    void disconnect() {
      m_connection.reset();
      m_idle = false;
      m_listactive = false;
    }

    bool connected() {
      if (!m_connection)
        return false;
      return m_connection.get() != nullptr;
    }

    bool retry_connection(int interval = 1) {
      if (connected())
        return true;

      while (true) {
        try {
          connect();
          return true;
        } catch (const mpd_exception& e) {
        }

        this_thread::sleep_for(chrono::duration<double>(interval));
      }

      return false;
    }

    int get_fd() {
      return m_fd;
    }

    void idle() {
      check_connection(m_connection.get());
      if (m_idle)
        return;
      mpd_send_idle(m_connection.get());
      check_errors(m_connection.get());
      m_idle = true;
    }

    int noidle() {
      check_connection(m_connection.get());
      int flags = 0;
      if (m_idle && mpd_send_noidle(m_connection.get())) {
        m_idle = false;
        flags = mpd_recv_idle(m_connection.get(), true);
        mpd_response_finish(m_connection.get());
        check_errors(m_connection.get());
      }
      return flags;
    }

    unique_ptr<mpdstatus> get_status() {
      check_prerequisites();
      auto status = make_unique<mpdstatus>(this);
      check_errors(m_connection.get());
      // if (update)
      //   status->update(-1, this);
      return status;
    }

    unique_ptr<mpdstatus> get_status_safe() {
      try {
        return get_status();
      } catch (const mpd_exception& e) {
        return {};
      }
    }

    unique_ptr<mpdsong> get_song() {
      check_prerequisites_commands_list();
      mpd_send_current_song(m_connection.get());
      details::mpd_song_t song{mpd_recv_song(m_connection.get()), details::mpd_song_deleter{}};
      mpd_response_finish(m_connection.get());
      check_errors(m_connection.get());
      if (song.get() != nullptr) {
        return make_unique<mpdsong>(std::move(song));
      }
      return unique_ptr<mpdsong>{};
    }

    void play() {
      try {
        check_prerequisites_commands_list();
        mpd_run_play(m_connection.get());
        check_errors(m_connection.get());
      } catch (const mpd_exception& e) {
        m_log.err("mpdconnection.play: %s", e.what());
      }
    }

    void pause(bool state) {
      try {
        check_prerequisites_commands_list();
        mpd_run_pause(m_connection.get(), state);
        check_errors(m_connection.get());
      } catch (const mpd_exception& e) {
        m_log.err("mpdconnection.pause: %s", e.what());
      }
    }

    void toggle() {
      try {
        check_prerequisites_commands_list();
        mpd_run_toggle_pause(m_connection.get());
        check_errors(m_connection.get());
      } catch (const mpd_exception& e) {
        m_log.err("mpdconnection.toggle: %s", e.what());
      }
    }

    void stop() {
      try {
        check_prerequisites_commands_list();
        mpd_run_stop(m_connection.get());
        check_errors(m_connection.get());
      } catch (const mpd_exception& e) {
        m_log.err("mpdconnection.stop: %s", e.what());
      }
    }

    void prev() {
      try {
        check_prerequisites_commands_list();
        mpd_run_previous(m_connection.get());
        check_errors(m_connection.get());
      } catch (const mpd_exception& e) {
        m_log.err("mpdconnection.prev: %s", e.what());
      }
    }

    void next() {
      try {
        check_prerequisites_commands_list();
        mpd_run_next(m_connection.get());
        check_errors(m_connection.get());
      } catch (const mpd_exception& e) {
        m_log.err("mpdconnection.next: %s", e.what());
      }
    }

    void seek(int songid, int pos) {
      try {
        check_prerequisites_commands_list();
        mpd_run_seek_id(m_connection.get(), songid, pos);
        check_errors(m_connection.get());
      } catch (const mpd_exception& e) {
        m_log.err("mpdconnection.seek: %s", e.what());
      }
    }

    void set_repeat(bool mode) {
      try {
        check_prerequisites_commands_list();
        mpd_run_repeat(m_connection.get(), mode);
        check_errors(m_connection.get());
      } catch (const mpd_exception& e) {
        m_log.err("mpdconnection.set_repeat: %s", e.what());
      }
    }

    void set_random(bool mode) {
      try {
        check_prerequisites_commands_list();
        mpd_run_random(m_connection.get(), mode);
        check_errors(m_connection.get());
      } catch (const mpd_exception& e) {
        m_log.err("mpdconnection.set_random: %s", e.what());
      }
    }

    void set_single(bool mode) {
      try {
        check_prerequisites_commands_list();
        mpd_run_single(m_connection.get(), mode);
        check_errors(m_connection.get());
      } catch (const mpd_exception& e) {
        m_log.err("mpdconnection.set_single: %s", e.what());
      }
    }

    operator details::mpd_connection_t::element_type*() {
      return m_connection.get();
    }

   protected:
    void check_prerequisites() {
      check_connection(m_connection.get());
      noidle();
    }

    void check_prerequisites_commands_list() {
      noidle();
      assert(!m_listactive);
      check_prerequisites();
    }

   private:
    const logger& m_log;
    details::mpd_connection_t m_connection;

    bool m_listactive = false;
    bool m_idle = false;
    int m_fd = -1;

    string m_host;
    unsigned int m_port;
    string m_password;
    unsigned int m_timeout;
  };

  // }}}
  // class: mpdstatus {{{

  class mpdstatus {
   public:
    explicit mpdstatus(mpdconnection* conn, bool autoupdate = true) {
      fetch_data(conn);
      if (autoupdate)
        update(-1, conn);
    }

    void fetch_data(mpdconnection* conn) {
      m_status.reset(mpd_run_status(*conn));
      m_updated_at = chrono::system_clock::now();
      m_songid = mpd_status_get_song_id(m_status.get());
      m_random = mpd_status_get_random(m_status.get());
      m_repeat = mpd_status_get_repeat(m_status.get());
      m_single = mpd_status_get_single(m_status.get());
      m_elapsed_time = mpd_status_get_elapsed_time(m_status.get());
      m_total_time = mpd_status_get_total_time(m_status.get());
    }

    void update(int event, mpdconnection* connection) {
      if (connection == nullptr || (event & (MPD_IDLE_PLAYER | MPD_IDLE_OPTIONS)) == false)
        return;

      fetch_data(connection);

      m_elapsed_time_ms = m_elapsed_time * 1000;

      auto state = mpd_status_get_state(m_status.get());

      switch (state) {
        case MPD_STATE_PAUSE:
          m_state = mpdstate::PAUSED;
          break;
        case MPD_STATE_PLAY:
          m_state = mpdstate::PLAYING;
          break;
        case MPD_STATE_STOP:
          m_state = mpdstate::STOPPED;
          break;
        default:
          m_state = mpdstate::UNKNOWN;
      }
    }

    void update_timer() {
      auto diff = chrono::system_clock::now() - m_updated_at;
      auto dur = chrono::duration_cast<chrono::milliseconds>(diff);
      m_elapsed_time_ms += dur.count();
      m_elapsed_time = m_elapsed_time_ms / 1000 + 0.5f;
      m_updated_at = chrono::system_clock::now();
    }

    bool random() const {
      return m_random;
    }

    bool repeat() const {
      return m_repeat;
    }

    bool single() const {
      return m_single;
    }

    bool match_state(mpdstate state) const {
      return state == m_state;
    }

    int get_songid() const {
      return m_songid;
    }

    unsigned get_total_time() const {
      return m_total_time;
    }

    unsigned get_elapsed_time() const {
      return m_elapsed_time;
    }

    unsigned get_elapsed_percentage() {
      if (m_total_time == 0)
        return 0;
      return static_cast<int>(float(m_elapsed_time) / float(m_total_time) * 100.0 + 0.5f);
    }

    string get_formatted_elapsed() {
      char buffer[32];
      snprintf(buffer, sizeof(buffer), "%lu:%02lu", m_elapsed_time / 60, m_elapsed_time % 60);
      return {buffer};
    }

    string get_formatted_total() {
      char buffer[32];
      snprintf(buffer, sizeof(buffer), "%lu:%02lu", m_total_time / 60, m_total_time % 60);
      return {buffer};
    }

    int get_seek_position(int percentage) {
      if (m_total_time == 0)
        return 0;
      math_util::cap<int>(0, 100, percentage);
      return float(m_total_time) * percentage / 100.0f + 0.5f;
    }

   private:
    details::mpd_status_t m_status;
    unique_ptr<mpdsong> m_song;
    mpdstate m_state = mpdstate::UNKNOWN;
    chrono::system_clock::time_point m_updated_at;

    bool m_random = false;
    bool m_repeat = false;
    bool m_single = false;

    int m_songid;

    unsigned long m_total_time;
    unsigned long m_elapsed_time;
    unsigned long m_elapsed_time_ms;
  };

  // }}}
}

LEMONBUDDY_NS_END
