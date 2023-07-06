# Lucci-UC7216T-Remote-Repl
Replacement for Lucci UC7216T Fan/Light remote control based on ATtiny84A.  This was my first ever electronics project and a lot of trial and error was done to make it work.

## Background
I made this remote control because two out of four in our house stopped working and the options to buy replacements online were too expensive for what they were.  The key part that made this difficult to build was that I insisted on putting a new PCB back into the original remote control, which directly influenced the design and difficulty of fabrication.

## The Original Remotes
The original remotes look like this:
<br>
<img src="/Images/Public/External%20Front.jpg" width="350">
<img src="/Images/Public/External%20Rear.jpg" width="350">
<img src="/Images/Public/Original%20PCB%20Front.jpg" width="350">
<img src="/Images/Public/Original%20PCB%20Rear.jpg" width="350">
<br>

They run 433 MHz transmitters and use a pair of AA batteries.

I used an RF receiver set up as a sniffer connected to Arduino serial via USB to learn the codes transmitted from a working remote for each button (light, high, medium, low, off) for each of the four fans in the house, using the toggle switches on the back of the remote to change between the different fans.

## The New Remotes
The software was written in Arduino 1.x.

As mentioned earlier, I used the original remote cases and button covers, etc for the new remotes, so externally nothing really has changed.

Inside I had to build a new circuit board using the parts listed below.  In order to fit the new circuit board a few things had to be changed:
* Some small bits of plastic inside the casing bottom half had to be carefully removed (not too much or the board will shake around inside).
* The silicone button cover had to be cut around the MCU.  It also had material removed very carefully over the new momentary switches so that they pressed and returned properly.
* Due to the location of the programming pins I had to remove the clip at the end of the battery cover, so it doesn't latch any more.  This isn't an issue because we don't take them out of their holsters on the wall very often and it's pretty secure even without it.
* I was able to remove the translucent silicone diffuser at the top because my LED fit perfectly in the hole in the case and this was easier than making it fit under the diffuser.

I located the programming pins so that they poked out where the toggle switches used to because I don't have to risk breaking the case to reprogram the remote.

For an antenna, I soldered a purple wire to the antenna terminal on the radio breakout and ran it straight-line down to the other end of the circuit board, where I soldered it to an empty hole to secure it.  I think at the time I calculated and checked the antenna length and it's approximately a multiple of the ideal length, but I don't remember for sure.

### Parts List
All the parts had to be carefully chosen to ensure that they fit within the dimensions of my case.  I used KICAD to double check that everything physically fit together before purchasing.  Also, at the time I didn't have the knowledge of SMD's or the ability or equipment to solder them, so everything is THT.
* Batteries: Eneloop AAA's (Although the photos show non-rechargeable batteries, normally the remotes run on Eneloops. Note they're slightly lower rated voltage)
* MCU: ATtiny84A-PU (the *A model runs at a lower voltage than the non-*A model, which was necessary given my battery choice)
* Transmitter: HopeRF RFM110 (Factory set to 433 MHz)
* Transistors: 2N3904
* Capacitors: 10uf 16V Tantalum; 100nf 50V Ceramic
* Diodes: 1N5819
* LED: Vishay TLHR4400-AS12Z
* Switches: Pololu-1400 (2-Pin, SPST, 50mA; very low profile)

### Schematic
<br>
<img src="/Images/Public/Schematic.png" height="600">
<br>

### Photos
<br>
<img src="/Images/Public/External-Rear-Cover-Off.jpg" width="600">
<img src="/Images/Public/Internal-Assembled.jpg" height="600">
<img src="/Images/Public/Internal-Side-View.jpg" width="600">
<img src="/Images/Public/New-PCB-Top.jpg" height="600">
<img src="/Images/Public/New-PCB-Bottom.jpg" height="600">
<br>
