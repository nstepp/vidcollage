/*
	vidcollage - command line video compositor

    Copyright (C) 2019  Nigel D. Stepp

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    Nigel Stepp <stepp@atistar.net>
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>

#define CV_FOURCC_STR(c) CV_FOURCC((c)[0],(c)[1],(c)[2],(c)[3])

#define CHECK_TILE_DESC(p) \
		if( !strlen(p) ) {\
			fprintf(stderr, "Malformed tile description\n");\
			usage();\
			exit(1);\
		}


using namespace std;
using namespace cv;

typedef struct tile_info_t {
	char *vid_filename;
	char *title;
	Rect tile_rect;
	int frames;
} tile_info_t;

tile_info_t *parse_tile(char *tile_spec);
void print_tile(tile_info_t *tile);
void usage();

int main(int argc, char **argv)
{
	int i;
	int fps = 30;
	Mat comp_frame;
	VideoWriter video_writer;
	vector<tile_info_t *> tiles; 
	char *output_filename = NULL;
	char *codec = NULL;
	bool verbose = false;

	int opt = 0, option_index = 0;

	struct option long_options[] = {
		{"codec", 1, 0, 'c'},
		{"fps", 1, 0, 'f'},
		{"output", 1, 0, 'o'},
		{"verbose", 0, 0, 'v'},
		{"help", 0, 0, 'h'},
		{0,0,0,0}
	};

	while( (opt = getopt_long(argc, argv, "c:f:o:vh",
			long_options, &option_index)) != -1 ) {

		switch(opt) {
			case 'c':
				codec = strdup(optarg);
				break;
			case 'f':
				fps = atoi(optarg);
				break;
			case 'o':
				output_filename = strdup(optarg);
				break;
			case 'v':
				verbose = true;
				break;
			case '?':
			case 'h':
			default:
				usage();
				exit(1);
		}
	}

	// parse video tile specifications
	int num_tiles = 0;
	for( i=optind; i<argc; i++ ) {
		tile_info_t *tile = parse_tile(argv[i]);

		if( verbose ) {
			printf("Adding tile %d: ", num_tiles);
			print_tile(tile);
		}

		tiles.push_back(tile);

		num_tiles++;
	}

	if( num_tiles <= 0 ) {
		usage();
		exit(1);
	}

	if( !codec ) {
		codec = strdup("xvid");
	}

	if( strlen(codec) != 4 ) {
		fprintf(stderr, "Codec must be a FOURCC identifier (www.fourcc.org)\n");
		exit(1);
	}

	if( fps < 1 ) {
		fprintf(stderr, "FPS must be a positive integer\n");
		exit(1);
	}

	VideoCapture *vids = new VideoCapture[num_tiles];
	Mat *frames = new Mat[num_tiles];

	int width = 0;
	int height = 0;
	int x_max = 0;
	int y_max = 0;
	int max_frames = 0;
	for( i=0; i<num_tiles; i++ ) {
		Rect r = tiles[i]->tile_rect;
		vids[i].open(tiles[i]->vid_filename);

		tiles[i]->frames = vids[i].get(CV_CAP_PROP_FRAME_COUNT);

		if( tiles[i]->frames > max_frames ) {
			max_frames = tiles[i]->frames;
		}
		if( r.x + r.width > x_max ) {
			x_max = r.x + r.width;
		}
		if( r.y + r.height > y_max ) {
			y_max = r.y + r.height;
		}

	}


	if( verbose ) {
		printf("Calculated final composite size %dx%d, %d frames\n",x_max, y_max, max_frames);
	}

		
	if( !output_filename ) {
		output_filename = strdup("composite.avi");
	}
	video_writer.open(output_filename, CV_FOURCC_STR(codec), fps, Size(x_max,y_max));

	if( !video_writer.isOpened() ) {
		fprintf(stderr, "\nFailed to open video writer, check filename, codec, and fps\n");
		exit(1);
	}

	int decade = 0;
	comp_frame = Mat(y_max, x_max, CV_8UC3);
	for( int frame=0; frame<max_frames; frame++ ) {
		for( i=0; i<num_tiles; i++ ) {
			if( tiles[i]->frames > frame ) {
				vids[i].read(frames[i]);
				Rect r = tiles[i]->tile_rect;
				resize(frames[i], comp_frame(r), Size(r.width, r.height), 0, 0, INTER_CUBIC);
			}
		}
	
		if( verbose ) {
			if( frame*10/max_frames > decade ) {
				decade++;
				printf("...%d%%", decade*10);
				fflush(stdout);
			}
		}

		video_writer.write(comp_frame);
	}
	if( verbose ) {
		printf("...100%%\n");
	}

	for( i=0; i<num_tiles; i++ ) {
		vids[i].release();
	}

	return 0;
}

tile_info_t *parse_tile(char *tile_spec)
{
	char *p = NULL;
		
	char *vid_file = strdup( strtok_r(tile_spec, "@", &p) );
	CHECK_TILE_DESC(p)

	int width = atoi( strtok_r(NULL, "x", &p) );
	CHECK_TILE_DESC(p)

	int height = atoi( strtok_r(NULL, "+", &p) );
	CHECK_TILE_DESC(p)

	int x_off = atoi( strtok_r(NULL, "+", &p) );
	CHECK_TILE_DESC(p)

	int y_off = atoi( p );

	tile_info_t *tile = (tile_info_t *)malloc(sizeof(tile_info_t));

	tile->vid_filename = vid_file;
	tile->tile_rect = Rect(x_off, y_off, width, height);
	tile->title = NULL;
	tile->frames = 0;

	return tile;
}

void print_tile(tile_info_t *tile)
{
	printf("%s@%dx%d+%d+%d\n", tile->vid_filename, tile->tile_rect.width, tile->tile_rect.height, tile->tile_rect.x, tile->tile_rect.y);
}

void usage()
{

	printf("\nvidcollage [-cfovh] tile_spec [tile_spec ...]\n\n");
	printf("\ttitle_spec := video_filename@WxH+X+Y\n\n");
	printf("\t-c,--codec FOURCC   Output video codec (default: XVID)\n");
	printf("\t-f,--fps fps        Output frames per second (default: 30)\n");
	printf("\t-o,--output output  Output filename\n");
	printf("\t-v,--verbose        Be verbose\n");
	printf("\t-h,--help           This help info\n");
	printf("\n");

}

