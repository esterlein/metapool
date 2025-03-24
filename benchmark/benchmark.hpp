class Benchmark
{
public:

	virtual ~Benchmark() {}

	virtual void setup() = 0;
	virtual void run() = 0;
	virtual void teardown() = 0;
};
