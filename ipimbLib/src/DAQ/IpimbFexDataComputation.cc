
// 1st Part - IpmFex Config Parameters:

IpmFexConfigV2 {
  enum { NCHANNELS=4 };
  DiodeFexConfigV2 diode[NCHANNELS];
  float xscale;
  float yscale;
}

	// Where - DiodeFexConfigV2 is:
	DiodeFexConfigV2 {
	  enum { NRANGES=8 };
	  float base [NRANGES];
	  float scale[NRANGES];
	}

//2nd part - Data read from Ipimb Box (4 channels voltage) -- showing only relevant data 
IpimbDataV2 {
  float channel0Volts;
  float channel1Volts;
  float channel2Volts;
  float channel3Volts;
}

//-----------------------------------------------------
// Computation Results = IpmFexV1 Data computed from 1st and 2nd part components:
IpmFexDataV1 {
  float channel[4];
  float sum;
  float xpos;
  float ypos;
}

//Where calculations for above part are as follows:
//Capacitor Range selected can be an integer ranging from 0 to 7 which acts as index for "base" and "scale" config parameters for each channels
//capacitor settings e.g asuming it as N. 

channel[0] = (diode[0].base[N] - channel0Volts) * diode[0].scale[N];
channel[1] = (diode[1].base[N] - channel1Volts) * diode[1].scale[N];
channel[2] = (diode[2].base[N] - channel2Volts) * diode[2].scale[N];
channel[3] = (diode[3].base[N] - channel3Volts) * diode[3].scale[N];

sum = channel[0] + channel[1] + channel[2] + channel[3];
xpos = xscale* (channel[1] - channel[3])/(channel[1] + channel[3] );
ypos = yscale* (channel[0] - channel[2])/(channel[0] + channel[2] );

//-------------------------------------------------------------------






