#include "point_search.h"

#define USE_CPP
#ifdef USE_CPP
#include <vector>
#include <algorithm> /* for std:sort */
#else
#include <string.h>  /* for memcpy   */
#include <stdlib.h>  /* for qsort    */
#endif

/* Declaration of the struct that is used as the context for the calls. */
struct SearchContext {
	size_t count;
#ifdef USE_CPP
	std::vector<Point> points;
#else
	Point * points;
#endif
};

/* comparison functions for sort routines resulting in a sort from smallest to largest rank */
#ifdef USE_CPP
bool pointsSortPredicate(const Point &a, const Point &b)
{
	return a.rank < b.rank;
}
#else
extern "C" int __cdecl pointsComparison(const void *a, const void *b)
{
	const Point *A = (const Point *)a;
	const Point *B = (const Point *)b;
	return A->rank - B->rank;
}
#endif

/* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
"points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
consecutive searches on the data. */
extern "C" SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	/* create a new context */
	SearchContext *sc = new SearchContext;
	/* determine how many total points */
	sc->count = (size_t)((uint8_t *)points_end - (uint8_t *)points_begin)/sizeof(Point);
#ifdef USE_CPP
	/* size our vector so won't have to reallocate memory */
	sc->points.reserve(sc->count);
	/* copy to our vector since no guarentee source to be valid after this call */
	sc->points.assign(points_begin, points_end);
	/* sort by rank, so can tranverse from lowest ranked Points to higher ones */
	std::sort(sc->points.begin(), sc->points.end(), pointsSortPredicate);
#else
	/* allocate big enough array to hold them all */
	sc->points = new Point[sc->count];
	/* copy to our array since no guarentee source to be valid after this call */
	memcpy(sc->points, points_begin, sc->count*sizeof(Point));
	/* sort by rank, so can tranverse from lowest ranked Points to higher ones */
	qsort(sc->points, sc->count, sizeof(Point), pointsComparison);
#endif
	/* return our context */
	return sc;
}



/* Search for "count" points with the smallest ranks inside "rect" and copy them ordered by smallest rank first in
"out_points". Return the number of points copied. "out_points" points to a buffer owned by the caller that
can hold "count" number of Points. */
extern "C" int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	/* keep track of matches found, initially none */
	int32_t matches = 0;
	/* search for first count matches */
#if 0
	/* valid for both vector or array sc->points, but slower */
	for (size_t i = 0; i < sc->count; i++)
	{
		/* next Point */
		const Point &p = sc->points[i];
#elif defined USE_CPP
	for (std::vector<Point>::const_iterator c_iter = sc->points.begin(); c_iter != sc->points.end(); ++c_iter)
	{
		/* next Point */
		const Point &p = (*c_iter);
#else
	for (const Point *pCur = sc->points, *pEnd = sc->points+sc->count; pCur < pEnd; ++pCur)
	{
		/* next Point */
		const Point &p = *pCur;
#endif
		/* does current Point lie within specified rect? */
		if ((p.x >= rect.lx) && (p.x <= rect.hx) && (p.y >= rect.ly) && (p.y <= rect.hy))
		{
			/* yes, so add to ones returned */
			out_points[matches] = p;
			/* update how many we have found so far */
			matches++;
			/* and if we hit the limit prior to going through all possible Points, exit early */
			if (matches >= count) break;
		}
	}
	return matches;
}

/* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
extern "C" SearchContext* __stdcall destroy(SearchContext* sc)
{
	/* free allocated memory */
#ifdef USE_CPP
	sc->points.clear();
	sc->points.swap(sc->points);
#else
	delete[] sc->points;
#endif
	delete sc;
	return nullptr;
}
