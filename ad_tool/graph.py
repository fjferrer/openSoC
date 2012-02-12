#!/usr/bin/env python
#
# Library for generate the plots
#

from scitools.std import *
from scitools.easyviz import *

class plt(object):
    
    """ Wrapper class to subplot/plot method in scitool pkg.
    This function is to create specific A/D plots."""

    def __init__(self,probes,measure="voltage",enablegrid=True):
        """ Init function """

        

        # Set interactive mode on
        setp(interactive=True)

        # Create the figure
        fig = figure()

        # Set X variable 'time' in zero
        xtime = 0
    
        self.ylabel('voltage (mV)')
        # Enable/Disable grid
        if enablegrid:
            grid('on')

    def show_plot(self):
        show()



if __name__ == "__main__":




    import pdb; pdb.set_trace()

    setp(interactive=False)
    a = figure()
    
    t = linspace(0,1,51)
    y1 = sin(2*pi*t)
    y2 = cos(2*pi*3*t)
    
    subplot(3,1,1)
    plot(t, y1, 'ro-')
    hold('on')
    plot(t, y2, 'b--')
    title('This is the title')
    ylabel('voltage (mV)')
    axis([0, 1, -1.5, 1.5])
    
    raw_input("press enter")
#
#    subplot(3,1,2)
#    plot(t, y1+y2, 'm:')
#    axis([0, 1, -3, 3])
#    grid('on')
#    xlabel('time (sec)')
#    ylabel('voltage (mV)')
#
#    subplot(3,1,3)
#    plot(t, y1+y2, 'm:')
#    axis([0, 1, -3, 3])
#    grid('on')
#    xlabel('time (sec)')
#    ylabel('voltage (mV)')
#
#    show()
#
