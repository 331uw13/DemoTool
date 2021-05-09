#include "demotool.h"


int main() {
	dt_start("demo");
	double timeline[] = {
		8.0,
		8.5,
		10.0,
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

