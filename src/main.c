#include "demotool.h"





int main() {
	dt_start("demo");


	double timeline[] = {

		1.0,
		1.0,
		1.0,
		1.0

	};


	dt_play(
			timeline,
			sizeof(timeline) / sizeof *timeline,
			"vertex.glsl",
		   	"shader.glsl",
		   	"dnb.wav"
	);
	
	return 0;
}


