// module is loaded and evaluated
// print("\nInitializing JS challenger!");

// create any global variables desired (try to minimize)

// any misc helper functions (modules not currently supported, so prepend any utility functions, etc.

// create 3 functions corresponding to 3 C challenger plugin functions, create & search



/* Initialize any internal structures from provided points (array of doubles*).
   The points are only guarenteed to be valid for the duration of the call [may be garbage collected
   if no other references created].
   Return an object with the context that can be used for consecutive searches on the data. */
function create(points) {
    /* quicksort our array of Points by rank */
    function pointsComparison(a, b) { return a.rank - b.rank; }

    var sc = {};
    sc.points = points.sort(pointsComparison);
    return sc;
}


/* Search for "count" points with the smallest ranks inside "rect" and return them ordered by smallest rank first in
   an array of [0 up to count] points. 
   Args are:
     this - search context returned from create(...);
     rect - a structure with { lx: ?, ly: ?, hx: ?, hy: ? } where lx,ly is low point and hx,hy is high point of rectangular region to find matches in
     count - max points to return
   */
function search(rect, count) {
    /* keep track of matches found, initially none */
    var matches = 0;
    var out_points = [];
    try {
        /* validate and adjust rect if needed */
        if (rect.hx < rect.lx) { var tx = rect.hx; rect.hx = rect.lx; rect.lx = tx; }
        if (rect.hy < rect.ly) { var ty = rect.hy; rect.hy = rect.ly; rect.ly = ty; }
        /* get our sorted (by rank) list of points */
        var points = this.points;
        /* search for first count matches */
        for (var i = 0; i < points.length; i++) {
            /* next Point */
            var p = points[i];
            /* does current Point lie within specified rect? */
            if ((p.x >= rect.lx) && (p.x <= rect.hx) && (p.y >= rect.ly) && (p.y <= rect.hy)) {
                /* yes, so add to ones returned */
                out_points[matches] = p;
                /* update how many we have found so far */
                matches++;
                /* and if we hit the limit prior to going through all possible Points, exit early */
                if (matches >= count) break;
            }
        }
    } catch (err) {
        print("ERROR:" + err);
    //} finally {
    //    print("returning " + matches + " points!");
    }
    return out_points;
}


/* clean up any external [to javascript] resources */
function destroy(searchContext) {
    return 0;
}
