# vidcollage
Command line video compositor

This is a command line utility for combining video files by placing each video at a specified region in a final composite video.

```
vidcollage [-cfovh] tile_spec [tile_spec ...]

	title_spec := video_filename@WxH+X+Y

	-c,--codec FOURCC   Output video codec (default: XVID)
	-f,--fps fps        Output frames per second (default: 30)
	-o,--output output  Output filename
	-v,--verbose        Be verbose
	-h,--help           This help info
```

## Example

Say you have `video1.avi` and `video2.avi`, which are both 1920x1080 videos. You can combine them so that they are playing side-by-side with the command:

    vidcollage -o twovids.avi video1.avi@960x540+0+0 video2.avi@960x540+960+0

The resulting video will be 1920x540, with `video1.avi` on the left and `video2.avi` on the right.
