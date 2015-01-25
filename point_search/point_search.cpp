#include <queue>
#include <random>
#include <stdio.h>
#include <string>
#include <tchar.h>
#include <vector>
#include <Windows.h>

#include "point_search.h"
#include "processInfo.h"
#include "timer.h"

/* seed our random number generator - see https://msdn.microsoft.com/en-us/library/aa387694.aspx */
#define SystemFunction036 NTAPI SystemFunction036
#include <NTSecAPI.h>
#undef SystemFunction036


/* for ASCII/Unicode strings depending on how compiled */
typedef std::basic_string<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR> > tstring;

typedef SearchContext *SearchContextPtr;

/* exported functions from challenger plugins */
struct PointFunctions {
	T_create create;
	T_search search;
	T_destroy destroy;
};

/* challenger plugin specific information */
struct Challenger {
	tstring name;        /* filename with path, & displayed identity */
	HMODULE handle;      /* handle to plugin (DLL) after loaded      */
	PointFunctions fns;  /* functions we import from plugin          */
	/* stored results for comparisons                                */
	Point * results;
	/* and stored process state for usage information                */
	processInfo pi;
};

/* normally runtime is positive, so flag a crash as negative time */
const double CRASHED_TIME = -1.0;

/* stores results information about a challengers runs */
struct ChallengerResults {
	double searchTime;     /* how long challenger took to run search */
	struct Challenger *challenger;

	ChallengerResults(Challenger *pChallenger) {
		searchTime = CRASHED_TIME;
		challenger = pChallenger;
	}
};

/* challenger results ranked for priority queue */
class RankChallengerResults {
public:
    bool operator()(ChallengerResults& c1, ChallengerResults& c2)
    {
		/* Note: if challenger crashed then will appear to be winner due to negative time */
		if (c1.searchTime > c2.searchTime) return true;
		return false;
    }
};

typedef std::priority_queue<ChallengerResults, std::vector<ChallengerResults>, RankChallengerResults> Rankings;

/* challenge specific setup */
struct ChallengeOptions {
	int32_t pointCount;
	int32_t queryCount;
	int32_t resultCount;
	int32_t randomSeed[4];
	std::vector<Challenger> plugins;
	std::vector<Point> points;
	std::vector<Rect> queryRects;
};


std::default_random_engine generator;
std::uniform_real_distribution<double> rdistribution(-0.5, 0.5);
std::uniform_int_distribution<int32_t> idistribution(INT_MIN/2,INT_MAX/2);


/* displays welcome/description banner to user */
void print_welcome_message(void)
{
	printf("--- Running Up A Hill Point Search Challenge ---\n"); 
}

/* sets default challenge options */
void initialize_default_options(ChallengeOptions &options)
{
	options.pointCount = 10000000;
	options.queryCount = 1000;
	options.resultCount = 20;

	RtlGenRandom(options.randomSeed, 16);
	std::seed_seq seed(&options.randomSeed[0],&options.randomSeed[3]);
	generator.seed(seed);

	options.plugins.clear();
	options.points.clear();
	options.queryRects.clear();
}

/* displays current option values */
void print_options(ChallengeOptions &options)
{
	printf("Point count  : %u\n", options.pointCount);
	printf("Query count  : %u\n", options.queryCount);
	printf("Result count : %u\n", options.resultCount);
	printf("Random seed  : %08X-%08X-%08X-%08X\n", options.randomSeed[0], options.randomSeed[1], options.randomSeed[2], options.randomSeed[3]);
	printf("\n");
}

/* displays usage information and terminates program */
void print_help_message(ChallengeOptions &options)
{
	printf(
		"Description: Given [point count] ranked points on a plane, find the \n"
		"[result count] most important points inside [query count] rectangles.  \n"
		"You can specify a list of plugins that solve this problem, and their \n"
		"results and performance will be compared!\n"
		"Usage:\n"
		"        point_search.exe plugin_paths [-pN] [-qN] [-rN] [-s]\n"
		"Options:\n"
		"        -pN: point count (default: %u)\n"
		"        -qN: query count (default: %u)\n"
		"        -rN: result count (default: %u)\n"
		"        -sX: specify seed (default: random)\n"
		"Example:\n"
		"        point_search.exe reference.dll coyote.dll -p10000000 -q100000 -r20 \n"
		"                         -s%08X-%08X-%08X-%08X\n",
		options.pointCount, options.queryCount, options.resultCount,
		options.randomSeed[0], options.randomSeed[1], options.randomSeed[2], options.randomSeed[3]
	);

	exit(1);
}

/* process any command line arguments, and updates options with user selected values          *
   if invalid argument or not enough arguments, then prints usage help and terminates program */
void process_command_line_arguments(int argc, TCHAR *argv[], ChallengeOptions &options)
{
	if (argc <= 1) print_help_message(options);

	for (int i = 1; i < argc; ++i)
	{
		if (*argv[i] == '-') {
			/* process options */
			switch(tolower(argv[i][1])) 
			{
				case 'p': {
					options.pointCount = _ttoi(argv[i]+2);
					break;
				}
				case 'q': {
					options.queryCount = _ttoi(argv[i]+2);
					break;
				}
				case 'r': {
					options.resultCount = _ttoi(argv[i]+2);
					break;
				}
				case 's': {
					printf("\n-s option not yet supported.  Sorry.\n\n");
					break;
				}
				default: {
					/* ignore invalid options */
					//print_help_message(options);  /* never returns to here */
				}
			}
		} else {
			/* assume its a plugin */
			Challenger c;
			c.name.append(argv[i]);
			options.plugins.push_back(c);
		}
	}
}


/* attempts to load a plugin and obtain pointer to required functions *
 * returns true if any error loading plugin or obtaining pointers     */
bool load_plugin(Challenger &plugin)
{
	_tprintf(_T("Loading %s... "), plugin.name.c_str());

	try {
		HMODULE h = LoadLibraryEx(plugin.name.c_str(), NULL, 0x200 /*LOAD_LIBRARY_SEARCH_APPLICATION_DIR*/);
		if (h == NULL) {
			printf("Not Found.\n");
			return true;
		}

		plugin.fns.create = (T_create)GetProcAddress(h, "create");
		plugin.fns.search = (T_search)GetProcAddress(h, "search");
		plugin.fns.destroy = (T_destroy)GetProcAddress(h, "destroy");
		if (plugin.fns.create == NULL || plugin.fns.search == NULL || plugin.fns.destroy == NULL) {
			printf("Not a valid module.\n");
			return true;
		}
	} catch(std::exception e) {
		printf("CRASHED!\n");
		return true;
	}

	printf("Success.\n");
	return false;
}

/* load all user requested plugins */
void load_all_plugins(ChallengeOptions &options)
{
	printf("Loading algorithms:\n");

	for (auto plugin=options.plugins.begin(); plugin!=options.plugins.end(); )
	{
		/* remove from list if failed to load correctly, otherwise advance */
		if (load_plugin(*plugin)) 
			plugin = options.plugins.erase(plugin);
		else
			++plugin;
	}

	if (options.plugins.size() == 0) 
		printf("No");
	else 
		printf("%d", options.plugins.size());
	printf(" algorithms loaded.\n\n");
}


/* returns random float between -INT_MAX/2 and INT_MAX/2 */
inline float frand(void)
{
	register float f = static_cast<float>(rdistribution(generator) * 99997.7);  // float only has FLT_DIG=6 digits of precision
	//printf("%+13.6f\n", (double)f);
	return f;
}


/* create list of random points to search */
void generate_random_points(size_t count, std::vector<Point> &points)
{
	ps_timer timer;
	printf("Preparing %d random points...", count);

	/* ensure empty and enough space for all our generated points */
	points.clear();
	points.reserve(count);

	/* create count points */
	for (size_t i=0; i < count; i++)
	{
		Point point;
		point.id = idistribution(generator) % 256;
		point.rank = idistribution(generator);
		point.x = frand();
		point.y = frand();
		points.push_back(point);
	}
	printf("done (%.4fms).\n", timer.elapsed());
}


/* create list of query rectangles for searches */
void generate_random_query_rects(size_t count, std::vector<Rect> &queryRects)
{
	ps_timer timer;
	printf("Preparing %d random queries...", count);

	/* ensure empty and enough space for all our generated points */
	queryRects.clear();
	queryRects.reserve(count);

	/* create count points */
	for (size_t i=0; i < count; i++)
	{
		Rect rect;
		rect.lx = frand();
		rect.hx = frand();
		rect.ly = frand();
		rect.hy = frand();
		/* ensure low and high values are in correct members */
		if (rect.hx < rect.lx) { float tx = rect.hx; rect.hx = rect.lx; rect.lx = tx; }
		if (rect.hy < rect.ly) { float ty = rect.hy; rect.hy = rect.ly; rect.lx = ty; }
		queryRects.push_back(rect);
	}
	printf("done (%.4fms).\n", timer.elapsed());
}


/* verify some basic funtionality of plugin *
 * returns true if any errors/failures      */
bool plugin_ruggedness_check(Challenger &plugin, ChallengeOptions &options)
{
	try {
		/* attempt running with no points */
		printf("Ruggedness check...");
		SearchContext *sc = plugin.fns.create(NULL, NULL);
		plugin.fns.search(sc, options.queryRects[0], options.resultCount, NULL);
		plugin.fns.destroy(sc);
	} catch(std::exception e) {
		printf("CRASHED!\n");
		return true;
	}
	printf("done.\n");
	return false;
}

/* pass a copy of random points to plugin *
 * returns true if any errors/failures    */
bool plugin_load_points(Challenger &plugin, SearchContextPtr &sc, ChallengeOptions &options)
{
	ps_timer timer;
	try {
		printf("Loading points...");
		sc = plugin.fns.create(options.points.data(), options.points.data()+options.pointCount);
	} catch(std::exception e) {
		printf("CRASHED!\n");
		return true;
	}
	printf("done (%.4fms).\n", timer.elapsed());
	return false;
}

/* do the queries                         *
 * returns ChallengerResults              */
ChallengerResults plugin_make_queries(Challenger &plugin, SearchContextPtr &sc, ChallengeOptions &options)
{
	ps_timer timer(false);
	ChallengerResults cResults(&plugin);;

	try {
		printf("Making queries...");
		int32_t index = 0;
		for (auto query = options.queryRects[index]; index < options.queryCount; ++index)
		{
			/* let challenger run the search and store found points */
			timer.start();
			plugin.fns.search(sc, query, options.resultCount, plugin.results+(index * options.resultCount));
			timer.stop();
		}
	} catch(std::exception e) {
		printf("CRASHED!\n");
		cResults.searchTime = CRASHED_TIME;
		return cResults;
	}

	cResults.searchTime = timer.elapsed();
	printf("done (%.4fms, avg %.4fms/query).\n", cResults.searchTime, cResults.searchTime/options.queryCount);
	return cResults;
}

/* cleanup */
void plugin_release_points(Challenger &plugin, SearchContextPtr &sc)
{
	plugin.pi.stop(); /* take snapshot prior to releasing resources */
	try {
		printf("Release points...");
		sc = plugin.fns.destroy(sc);
		if (sc != nullptr) { printf("Failed to destroy SearchContext.\n"); }
	} catch(std::exception e) {
		printf("CRASHED!\n");
	}
	printf("done (memory used: %dMB).\n", plugin.pi.usedMB());
}


int _tmain(int argc, TCHAR* argv[])
{
	ChallengeOptions options;
	Rankings rankings;

	print_welcome_message();
    initialize_default_options(options);
	process_command_line_arguments(argc, argv, options);
	print_options(options);
	load_all_plugins(options);

	if (options.plugins.size() > 0) {
		generate_random_points(options.pointCount, options.points);
		generate_random_query_rects(options.queryCount, options.queryRects);

		/* run the challenge */
		for (size_t i=0; i < options.plugins.size(); i++)
		{
			_tprintf(_T("\nTesting algorithm #%d (%s):\n"), i, options.plugins[i].name.c_str());
			/* reserve space for all searches with all query rects */
			options.plugins[i].results = new Point[options.queryCount * options.resultCount];
			/* snapshot memory so can obtain usage information */
			options.plugins[i].pi.start();
			SearchContext *sc;
			if (plugin_ruggedness_check(options.plugins[i], options)) continue;
			if (plugin_load_points(options.plugins[i], sc, options)) continue;
			ChallengerResults cResults = plugin_make_queries(options.plugins[i], sc, options);
			rankings.push(cResults);
			if (cResults.searchTime == CRASHED_TIME) continue;
			plugin_release_points(options.plugins[i], sc);
		}

		/* validate results */
		printf("\nComparing the results of algorithms:\n");
		printf("TODO!\n");

		/* print rankings */
		printf("\nScoreboard:\n");
		for (size_t rank = 0; !rankings.empty(); ++rank) {
			ChallengerResults cResults = rankings.top();
			_tprintf(_T("#%d: %.4fms %s\n"), rank, cResults.searchTime, cResults.challenger->name.c_str());
			rankings.pop();
		}

		/* any final cleanup */
		printf("\nCleaning up resources...");
		// do it
		printf("done.\n");
	}
	return 0;
}

