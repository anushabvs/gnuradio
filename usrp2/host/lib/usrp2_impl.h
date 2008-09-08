/* -*- c++ -*- */
/*
 * Copyright 2008 Free Software Foundation, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_USRP2_IMPL_H
#define INCLUDED_USRP2_IMPL_H

#include <usrp2/usrp2.h>
#include <usrp2/data_handler.h>
#include <usrp2_eth_packet.h>
#include <boost/scoped_ptr.hpp>
#include "control.h"
#include "ring.h"
#include <string>

namespace usrp2 {
  
  class eth_buffer;
  class pktfilter;
  class usrp2_thread;
  class usrp2_tune_result;
  class pending_reply;
  class ring;

  class usrp2::impl : private data_handler
  {
    static const size_t NRIDS = 256;
    static const size_t NCHANS = 32;

    eth_buffer    *d_eth_buf;
    pktfilter     *d_pf;
    std::string    d_addr;       // FIXME: use u2_mac_addr_t instead
    usrp2_thread  *d_bg_thread;
    volatile bool  d_bg_running; // TODO: multistate if needed
    
    int            d_rx_decim;
    int            d_rx_seqno;
    int            d_tx_seqno;
    int            d_next_rid;
    unsigned int   d_num_rx_frames;
    unsigned int   d_num_rx_missing;
    unsigned int   d_num_rx_overruns;
    unsigned int   d_num_rx_bytes;

    unsigned int   d_num_enqueued;
    omni_mutex     d_enqueued_mutex;
    omni_condition d_bg_pending_cond;

    // all pending_replies are stack allocated, thus no possibility of leaking these
    pending_reply *d_pending_replies[NRIDS]; // indexed by 8-bit reply id

    std::vector<ring_sptr>   d_channel_rings; // indexed by 5-bit channel number

    void inc_enqueued() {
      omni_mutex_lock l(d_enqueued_mutex);
      d_num_enqueued++;
    }
    
    void dec_enqueued() {
      omni_mutex_lock l(d_enqueued_mutex);
      if (--d_num_enqueued == 0)
        d_bg_pending_cond.signal();
    }
    
    static bool parse_mac_addr(const std::string &s, u2_mac_addr_t *p);
    void init_et_hdrs(u2_eth_packet_t *p, const std::string &dst);
    void init_etf_hdrs(u2_eth_packet_t *p, const std::string &dst,
		       int word0_flags, int chan, uint32_t timestamp);
    void stop_bg();
    void init_config_rx_v2_cmd(op_config_rx_v2_cmd *cmd);
    void init_config_tx_v2_cmd(op_config_tx_v2_cmd *cmd);
    bool transmit_cmd(void *cmd, size_t len, pending_reply *p, double secs=0.0);
    virtual data_handler::result operator()(const void *base, size_t len);
    data_handler::result handle_control_packet(const void *base, size_t len);
    data_handler::result handle_data_packet(const void *base, size_t len);

  public:
    impl(const std::string &ifc, props *p);
    ~impl();
    
    void bg_loop();

    std::string mac_addr() const { return d_addr; } // FIXME: convert from u2_mac_addr_t
    bool burn_mac_addr(const std::string &new_addr);

    bool set_rx_gain(double gain);
    bool set_rx_center_freq(double frequency, tune_result *result);
    bool set_rx_decim(int decimation_factor);
    bool set_rx_scale_iq(int scale_i, int scale_q);
    bool start_rx_streaming(unsigned int channel, unsigned int items_per_frame);
    bool rx_samples(unsigned int channel, rx_sample_handler *handler);
    bool stop_rx_streaming(unsigned int channel);
    unsigned int rx_overruns() const { return d_num_rx_overruns; }
    unsigned int rx_missing() const { return d_num_rx_missing; }

    bool set_tx_gain(double gain);
    bool set_tx_center_freq(double frequency, tune_result *result);
    bool set_tx_interp(int interpolation_factor);
    bool set_tx_scale_iq(int scale_i, int scale_q);

    bool tx_complex_float(unsigned int channel,
			  const std::complex<float> *samples,
			  size_t nsamples,
			  const tx_metadata *metadata);

    bool tx_complex_int16(unsigned int channel,
			  const std::complex<int16_t> *samples,
			  size_t nsamples,
			  const tx_metadata *metadata);

    bool tx_raw(unsigned int channel,
		const uint32_t *items,
		size_t nitems,
		const tx_metadata *metadata);
  };
  
} // namespace usrp2

#endif /* INCLUDED_USRP2_IMPL_H */
