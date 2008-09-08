#!/usr/bin/env python

"""
Read samples from the USRP2 and write to file formatted as binary
outputs single precision complex float values or complex short values (interleaved 16 bit signed short integers).

"""

from gnuradio import gr, eng_notation
from gnuradio import usrp2
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import sys

n2s = eng_notation.num_to_str

class rx_cfile_block(gr.top_block):

    def __init__(self, options, filename):
        gr.top_block.__init__(self)

        # Create a USRP2 source
        if options.output_shorts:
            self._u = usrp2.source_16sc(options.interface, options.mac_addr)
            self._sink = gr.file_sink(gr.sizeof_short*2, filename)
        else:
            self._u = usrp2.source_32fc(options.interface, options.mac_addr)
            self._sink = gr.file_sink(gr.sizeof_gr_complex, filename)

        # Set receiver decimation rate
        self._u.set_decim(options.decim)

        # Set receive daughterboard gain
        if options.gain is None:
            #g = self._u.gain_range()
            #options.gain = float(g[0]+g[1])/2
            options.gain = 0 # Until gain range is implemented
        self._u.set_gain(options.gain)

        # Set receive frequency
        tr = self._u.set_center_freq(options.freq)
        if tr == None:
            sys.stderr.write('Failed to set center frequency\n')
            raise SystemExit, 1

        # Create head block if needed wire it up
        if options.nsamples is None:
            self.connect(self._u, self._sink)
        else:
            if options.output_shorts:
                self._head = gr.head(gr.sizeof_short*2, int(options.nsamples))
            else:
                self._head = gr.head(gr.sizeof_gr_complex, int(options.nsamples))

            self.connect(self._u, self._head, self._sink)

        #input_rate = self.u.adc_freq() / self.u.decim_rate()
        input_rate = 100e6/options.decim
        
        if options.verbose:
            print "Network interface:", options.interface
            print "USRP2 address:", self._u.mac_addr()
            #print "Using RX d'board %s" % (self._u.rx_name(),)
            print "Rx gain:", options.gain
            print "Rx baseband frequency:", n2s(tr.baseband_freq)
            print "Rx DDC frequency:", n2s(tr.dxc_freq)
            print "Rx residual frequency:", n2s(tr.residual_freq)
            print "Rx decimation rate:", options.decim
            print "Rx sample rate:", n2s(input_rate)
            if options.nsamples is None:
                print "Receiving samples until Ctrl-C"
            else:
                print "Receving", n2s(options.nsamples), "samples"
            if options.output_shorts:
                print "Writing 16-bit complex shorts"
            else:
                print "Writing 32-bit complex floats"
            print "Output filename:", filename
        
def get_options():
    usage="%prog: [options] output_filename"
    parser = OptionParser(option_class=eng_option, usage=usage)
    parser.add_option("-e", "--interface", type="string", default="eth0",
                      help="use specified Ethernet interface [default=%default]")
    parser.add_option("-m", "--mac-addr", type="string", default="",
                      help="use USRP2 at specified MAC address [default=None]")  
    parser.add_option("-d", "--decim", type="int", default=16,
                      help="set fgpa decimation rate to DECIM [default=%default]")
    parser.add_option("-f", "--freq", type="eng_float", default=None,
                      help="set frequency to FREQ", metavar="FREQ")
    parser.add_option("-g", "--gain", type="eng_float", default=None,
                      help="set gain in dB (default is midpoint)")
    parser.add_option( "-s","--output-shorts", action="store_true", default=False,
                      help="output interleaved shorts instead of complex floats")
    parser.add_option("-N", "--nsamples", type="eng_float", default=None,
                      help="number of samples to collect [default=+inf]")
    parser.add_option("-v", "--verbose", action="store_true", default=False,
                      help="verbose output")
    (options, args) = parser.parse_args ()
    if len(args) != 1:
        parser.print_help()
        raise SystemExit, 1
    
    if options.freq is None:
        parser.print_help()
        sys.stderr.write('You must specify the frequency with -f FREQ\n');
        raise SystemExit, 1
    
    return (options, args[0])


if __name__ == '__main__':
    (options, filename) = get_options()
    tb = rx_cfile_block(options, filename)
    
    try:
        tb.run()
    except KeyboardInterrupt:
        pass
