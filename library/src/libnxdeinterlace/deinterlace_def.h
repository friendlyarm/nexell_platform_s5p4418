//////////////////////////////////////////////////////////////////////////////
//
//      Video DEINTERLACE DEFINE HEADER FILE
//

#include <nx_alloc_mem.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>     // malloc & free
#include <string.h>     // memset


// 15.08.19 initial version = 1.0.0
#define NX_DEINTERLACE_VER_MAJOR          1
#define NX_DEINTERLACE_VER_MINOR          0
#define NX_DEINTERLACE_VER_PATCH          0

#define MAX_FRAME_BUFFERS                 32
#define DEINTERLACE_DST_SIZE              3
#define PICTURE_PLANE_MAX                 5
#define HISTORY_SIZE                      3


typedef struct
{
	uint8_t *p_pixels;                   // Start of the plane's data

	// Variables used for fast memcpy operations
	//int i_lines;                       // Number of lines, including margins
	int i_pitch;                         // Number of bytes in a line, including margins

	// Variables used for pictures with margins
	int i_visible_lines;                 // How many visible lines are there ?
	int i_visible_pitch;                 // How many visible pixels are there ?
} plane_t;

typedef struct
{
	plane_t p[PICTURE_PLANE_MAX];        // description of the planes
	int32_t i_planes;                    // number of allocated planes

	uint32_t i_nb_fields;                // # of displayed fields
} picture_t;

typedef struct
{
	//const vlc_chroma_description_t *chroma;

	// Merge routine: C, MMX, SSE, ALTIVEC, NEON, ...
	void (*pf_merge) ( void *, const void *, const void *, size_t );

	// Output frame timing / framerate doubler control
	int32_t i_frame_offset;

	// Input frame history buffer for algorithms with temporal filtering.
	picture_t *pp_history[HISTORY_SIZE];

	// Algorithm-specific substructures
	//phosphor_sys_t phosphor; /**< Phosphor algorithm state. */
} filter_sys_t;

typedef struct
{
	filter_sys_t     *p_sys;
}  filter_t;

typedef struct
{
	int32_t iMode;
	filter_t *p_filter;
} NX_VIDEO_DEINTERLACE_INFO;
