#ifndef XAI_MAP_HDR
#define XAI_MAP_HDR

#include <list>
#include <vector>
#include "../main/XAIConstants.hpp"

enum XAIMapType {
	XAI_BASE_MAP   = -2,
	XAI_MASK_MAP   = -1,
	XAI_HEIGHT_MAP =  0,
	XAI_SLOPE_MAP  =  1,
	XAI_THREAT_MAP =  2,
};



template<typename T> struct XAIMapPixel {
public:
	XAIMapPixel(): x(-1), y(-1), v(T(0)) {}
	XAIMapPixel(int xp, int yp, T val): x(xp), y(yp), v(val) {}

	int GetX() const { return x; }
	int GetY() const { return y; }
	T GetValue() const { return v; }
	void SetX(int nx) { x = nx; }
	void SetY(int ny) { y = ny; }
	void SetValue(T nv) { v = nv; }

	XAIMapPixel<T>& operator += (T s) { v += s; return *this; }
	XAIMapPixel<T>& operator *= (T s) { v *= s; return *this; }
	XAIMapPixel<T>& operator /= (T s) { v /= s; return *this; }

private:
	// coordinates
	int x, y;

	// value
	T v;
};



template<typename T> struct XAIMap {
public:
	XAIMap(int sx, int sy, T v, XAIMapType t): type(t), mapx(sx), mapy(sy) {
		pixels.resize(sx * sy, XAIMapPixel<T>(-1, -1, v));
	}
	~XAIMap() {
		pixels.clear();
	}

	void Copy(const T* map) {
		for (int idx = (mapx * mapy) - 1; idx >= 0; idx--) {
			pixels[idx] = XAIMapPixel<T>((idx % mapx), (idx / mapx), map[idx]);
		}
	}

	// bool InBounds(int x, int y) const { return ((x >= 0 && x < mapx) && (y >= 0 && y < mapy)); }
	bool InBounds(int idx) const { return (idx >= 0 && idx < (mapx * mapy)); }
	bool InBounds(int x, int y) const { return (InBounds(y * mapx + x)); }
	bool InBounds(const XAIMapPixel<T>* p) const { return (InBounds(p->GetX(), p->GetY())); }

	XAIMapType GetType() const { return type; }
	int GetSizeX() const { return mapx; }
	int GetSizeY() const { return mapy; }


	// these are non-const, so caller may change pixel values
	XAIMapPixel<T>* GetPixel(int idx) {
		static XAIMapPixel<T> pxl(-1, -1, T(0));

		if (InBounds(idx)) {
			return (&pixels[idx]);
		} else {
			return (&pxl);
		}
	}
	XAIMapPixel<T>* GetPixel(int x, int y) {
		return (GetPixel(y * mapx + x));
	}

	// these are const, so caller may not set pixel values
	const XAIMapPixel<T>* GetConstPixel(int idx) const {
		static const XAIMapPixel<T> pxl(-1, -1, T(0));

		if (InBounds(idx)) {
			return (&pixels[idx]);
		} else {
			return (&pxl);
		}
	}
	const XAIMapPixel<T>* GetConstPixel(int x, int y) const {
		return (GetConstPixel(y * mapx + x));
	}

protected:
	XAIMapType type;

	int mapx;
	int mapy;

	std::vector<XAIMapPixel<T> > pixels;
};

#endif
