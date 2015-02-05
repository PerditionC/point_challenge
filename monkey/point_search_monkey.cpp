#include "point_search.h"
#include "duktape.h" /* note: DUK_OPT_NO_VOLUNTARY_GC defined to eliminate mark-and-sweep pauses */


/* Declaration of the struct that is used as the context for the calls. */
struct SearchContext {
	duk_context *js_ctx;	/* javascript (Duktape) interpreter context */
	duk_idx_t js_sc;		/* internal javascript SearchContext object */
};

/* converts a struct Point * into equivalent javascript object on top of duktape js interpreter stack, { id:#, rank:#, x:#, y:# }; */
static bool createPoint(duk_context * js_ctx, const struct Point *p) 
{
	// create object [...{Point}]
	duk_idx_t obj_idx = duk_push_object(js_ctx);
	// push member elements: id,rank,x,y   [...{Point}{#}{"x"}]-->[...{Point x:#}]
	duk_push_int(js_ctx, (int)p->id);
	if (!duk_put_prop_string(js_ctx, obj_idx, "id")) return false;
	duk_push_int(js_ctx, p->rank);
	if (!duk_put_prop_string(js_ctx, obj_idx, "rank")) return false;
	duk_push_number(js_ctx, (double)p->x);
	if (!duk_put_prop_string(js_ctx, obj_idx, "x")) return false;
	duk_push_number(js_ctx, (double)p->y);
	if (!duk_put_prop_string(js_ctx, obj_idx, "y")) return false;
	// return success
	return true;
}

/* invoke javascript create() function to initialize js challenger and let them put points in internal structure for later use */
static duk_idx_t call_js_create(duk_context * js_ctx, const Point* points_begin, const Point* points_end)
{
	// get js create(points) function [{create}]
	if (!duk_get_global_string(js_ctx, "create")) {
		printf("Error: Failed to find javascript create() function.\n");
		return DUK_INVALID_INDEX;
	}
	// push array of Points [{create}{points}]
	duk_idx_t points = duk_push_array(js_ctx);
	register duk_uarridx_t i=0;
	for (const Point* p=points_begin; p < points_end; ++p, ++i){
		if (createPoint(js_ctx, p))					// [{create}{points}{p}]
			duk_put_prop_index(js_ctx, points, i);  // [{create}{points[...,p]}]
	}
	// invoke create with 1 argument (the points array)
	if (duk_pcall(js_ctx, 1)) return DUK_INVALID_INDEX;
	/* don't duk_pop(sc->js_ctx), leave result on stack, its the javascript context for search */
	return duk_normalize_index(js_ctx, -1);  // convert to non-relative index for later use  [{js_ctx}]
}

static void call_js_destroy(SearchContext *sc)
{
	// destroy is optional, so see if it exists to call, leaving function object on stack if found [...{destroy}]
	if (!duk_get_global_string(sc->js_ctx, "destroy")) return; /* nothing to do */
	// push javascript search context [...{destroy}{sc}]
	duk_dup(sc->js_ctx, sc->js_sc);	
	// invoke create with 1 argument (the search context)
	duk_pcall(sc->js_ctx, 1);
	// ignore result 
	duk_pop(sc->js_ctx);
}

/* free's memory associated with SearchContext, sets to nullptr, and returns nullptr */
static inline SearchContext * cleanupSearchContext(SearchContext * &sc, const char *errMsg)
{
	printf("Error: %s\n", errMsg);
	delete sc;
	sc = nullptr;
	return nullptr;
}

/* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
"points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
consecutive searches on the data. */
extern "C" SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	/* create a new context */
	SearchContext *sc = new SearchContext;

	/* create our javascript (Duktape) context, TODO add error handler so error in javascript exits cleanly instead of aborting */
    sc->js_ctx = duk_create_heap_default();
    if (!sc->js_ctx) return cleanupSearchContext(sc, "Failed to create a Duktape heap.");
    if (duk_peval_file(sc->js_ctx, "challenger.js") != 0) return cleanupSearchContext(sc, duk_safe_to_string(sc->js_ctx, -1));
	duk_pop(sc->js_ctx);  /* ignore result */
	
	/* give javascript a chance to initialize and copy points as needed */
	sc->js_sc = call_js_create(sc->js_ctx, points_begin, points_end);
	if (sc->js_sc == DUK_INVALID_INDEX) return cleanupSearchContext(sc, "Javascript create(points); function call failed.");

	/* return our context */
	return sc;
}



/* place a copy of Rect onto javascript interpreter stack */
static bool createRect(duk_context * js_ctx,  const Rect &rect)
{
	// create the rect object
	duk_idx_t rect_idx = duk_push_object(js_ctx);
	// and store member values, lx, ...
	duk_push_number(js_ctx, (double)rect.lx);
	duk_put_prop_string(js_ctx, rect_idx, "lx");
	duk_push_number(js_ctx, (double)rect.ly);
	duk_put_prop_string(js_ctx, rect_idx, "ly");
	duk_push_number(js_ctx, (double)rect.hx);
	duk_put_prop_string(js_ctx, rect_idx, "hx");
	duk_push_number(js_ctx, (double)rect.hy);
	duk_put_prop_string(js_ctx, rect_idx, "hy");
	// success
	return true;
}

/* object representing array of points on stack, retrieve point_idx Point and copy into p */
static void getPoint(duk_context * js_ctx, Point *p, int32_t point_idx)
{
	/* get point_idx'th Point object on top of stack */
	if (duk_get_prop_index(js_ctx, -1, point_idx))  // [{out_points}{p}]
	{
		/* extract individual elements */
		if (duk_get_prop_string(js_ctx, -1, "id")) {		// [{out_points}{p}{id}]
			p->id = duk_to_int32(js_ctx, -1);
		}
		duk_pop(js_ctx);

		if (duk_get_prop_string(js_ctx, -1, "rank")){	// [{out_points}{p}{rank}]
			p->rank = duk_to_int32(js_ctx, -1);
		}
		duk_pop(js_ctx);

		if (duk_get_prop_string(js_ctx, -1, "x")){		// [{out_points}{p}{x}]
			p->x = (float)duk_to_number(js_ctx, -1);
		}
		duk_pop(js_ctx);

		if (duk_get_prop_string(js_ctx, -1, "y")){		// [{out_points}{p}{y}]
			p->y = (float)duk_to_number(js_ctx, -1);
		}
		duk_pop(js_ctx);
	} else {
		printf("Error: Expected array element not on stack!\n");
	}
	duk_pop(js_ctx);  // pop {p} object or {undefined} off stack
}


/* Search for "count" points with the smallest ranks inside "rect" and copy them ordered by smallest rank first in
"out_points". Return the number of points copied. "out_points" points to a buffer owned by the caller that
can hold "count" number of Points. */
extern "C" int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	/* run search */
	if (!duk_get_global_string(sc->js_ctx, "search")) {
		printf("Error: Failed to find javascript search() function.\n");
		return 0;
	}
	duk_dup(sc->js_ctx, sc->js_sc);	// push this = search context
	// push rect
	if (!createRect(sc->js_ctx, rect)) {
		printf("Error: Failed to copy rect to javascript.\n");
		return 0;
	}
	duk_push_int(sc->js_ctx, count);  // push max count of elements to return
	if (duk_pcall_method(sc->js_ctx, 2) != DUK_EXEC_SUCCESS) {
		printf("Error: Search() call failed.\n");
		return 0;
	}

	/* top of js context stack should be return value (ie array of points) from search(...) call [{out_points}] */
	if (duk_is_array(sc->js_ctx, -1))
	{
		/* retrieve how many Points returned, ie length of array returned */
		duk_get_prop_string(sc->js_ctx, -1, "length");  // [{out_points}{length}]
		int32_t matches = duk_to_int32(sc->js_ctx, -1); // [{out_points}{int32(length)}]
		//printf("search(...) returned %ld points\n", matches);
		duk_pop(sc->js_ctx); /* pop length off stack */ // [{out_points}]
		/* copy out results */
		register int32_t i = 0;
		for (Point *p = out_points; i < matches; ++p, ++i) {
			getPoint(sc->js_ctx, p, i);
			//printf("[%d]=%d,x=%.3f,y=%.3f  ", i, p->rank, (double)p->x, (double)p->y);
		}
		//printf("\n");
		return matches;
	} else {
		/* invalid value returned, so we return no results! */
		printf("invalid return value!\n");
		return 0;
	}
}

/* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
extern "C" SearchContext* __stdcall destroy(SearchContext* sc)
{
	/* let javascript code cleanup anything it needs to; optional */
	call_js_destroy(sc);
	/* free allocated memory */
	duk_destroy_heap(sc->js_ctx);
	delete sc;
	return nullptr;
}
