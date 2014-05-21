#ifndef XAI_RNG_HDR
#define XAI_RNG_HDR

// Mersenne-Twister RNG base (generates 32-bit random integers)
class RNGint32 {
public:
	// default constructor: uses default seed
	// only if this is the first instance
	RNGint32() {
		if (!init) {
			seedGen(5489UL);
			init = true;
		}
	}
	RNGint32(unsigned int s) {
		seedGen(s); init = true;
	}

	void seedGen(unsigned int);

	unsigned int operator () () {
		return genNextNum();
	}

protected:
	unsigned int genNextNum();

private:
	// compile time constants
	static const int n = 624, m = 397;

	// state vector array
	static unsigned int state[n];
	// position in state array
	static int p;
	static bool init;

	unsigned int twiddle(unsigned int, unsigned int);
	void genState();

	// copy constructor and assignment
	// oper. don't make sense to expose
	RNGint32(const RNGint32&);
	void operator = (const RNGint32&);
};

inline unsigned int RNGint32::twiddle(unsigned int u, unsigned int v) {
	return (((u & 0x80000000UL) | (v & 0x7FFFFFFFUL)) >> 1) ^ ((v & 1UL)? 0x9908B0DFUL: 0x0UL);
}

inline unsigned int RNGint32::genNextNum() {
	if (p == n) {
		// new state vector needed
		genState();
	}

	// genState() is split off to be non-inline, because it is only called once in
	// every 624 calls and otherwise irand() would become too big to get inlined
	unsigned int x = state[p++];

	x ^= (x >> 11);
	x ^= (x << 7) & 0x9D2C5680UL;
	x ^= (x << 15) & 0xEFC60000UL;

	return (x ^ (x >> 18));
}



// generates double-precision floating point
// numbers in the half-open interval [0, 1)
class RNGflt64: public RNGint32 {
public:
	RNGflt64(): RNGint32() {
	}
	RNGflt64(unsigned int seed): RNGint32(seed) {
	}
	~RNGflt64() {
	}

	double operator () () {
		return (static_cast<double>(genNextNum()) / 4294967296.0);
	}

private:
	RNGflt64(const RNGflt64&);
	void operator = (const RNGflt64&);
};

#endif
