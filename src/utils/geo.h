#pragma once

#include <array>
#include <iostream>

// two-dimensional point, v_[0] is x, v_[1] is y
template <typename T>
class Point2 {
private:
    array<T, 2> v_;

public:
    Point2() {}  // should be set
    inline void set(const T& x, const T& y) {
        v_[0] = x;
        v_[1] = y;
    }
    Point2(const T& x, const T& y) { set(x, y); }

    inline T& x() { return v_[0]; }
    inline T& y() { return v_[1]; }
    inline T& operator[](size_t i) {
        assert(i == 0 || i == 1);
        return v_[i];
    }
    inline const T& x() const { return v_[0]; }
    inline const T& y() const { return v_[1]; }
    inline const T& operator[](size_t i) const {
        assert(i == 0 || i == 1);
        return v_[i];
    }

    inline Point2 operator-(const Point2& rhs) { return Point2(v_[0] - rhs.v_[0], v_[1] - rhs.v_[1]); }
};

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const Point2<T>& p) {
    os << "(" << p[0] << ", " << p[1] << ")";
    return os;
}

//*****************************************************************************

// Two usages:
// 1. initialize by no point + update();
// 2. initialize by the first point + fupdate()
template <typename T>
class Box {
private:
    Point2<T> up_, lo_;

public:
    inline void set() {
        up_.set(numeric_limits<T>::lowest(), numeric_limits<T>::lowest()),
            lo_.set(numeric_limits<T>::max(), numeric_limits<T>::max());
    }
    inline void set(const T& x, const T& y) {
        up_.set(x, y);
        lo_.set(x, y);
    }
    inline void set(const T& ux, const T& uy, const T& lx, const T& ly) {
        up_.set(ux, uy);
        lo_.set(lx, ly);
    }
    Box() { set(); }
    Box(const T& x, const T& y) { set(x, y); }
    Box(const T& ux, const T& uy, const T& lx, const T& ly) { set(ux, uy, lx, ly); }

    inline T& ux() { return up_.x(); }
    inline T& uy() { return up_.y(); }
    inline T& lx() { return lo_.x(); }
    inline T& ly() { return lo_.y(); }
    inline T ux() const { return up_.x(); }
    inline T uy() const { return up_.y(); }
    inline T lx() const { return lo_.x(); }
    inline T ly() const { return lo_.y(); }
    inline T x() const { return up_.x() - lo_.x(); }
    inline T y() const { return up_.y() - lo_.y(); }
    inline T cx() const { return (up_.x() + lo_.x()) / 2; }
    inline T cy() const { return (up_.y() + lo_.y()) / 2; }
    inline T hp() const { return x() + y(); }
    inline T uhp() const { return 0.5 * x() + y(); }  // half perimeter under UltraScale metric
    inline void update(const T& x, const T& y) {
        if (x > up_.x()) up_.x() = x;
        if (x < lo_.x()) lo_.x() = x;
        if (y > up_.y()) up_.y() = y;
        if (y < lo_.y()) lo_.y() = y;
    }
    inline void fupdate(const T& x, const T& y) {
        if (x > up_.x())
            up_.x() = x;
        else if (x < lo_.x())
            lo_.x() = x;
        if (y > up_.y())
            up_.y() = y;
        else if (y < lo_.y())
            lo_.y() = y;
    }
    inline T dist(const T& x, const T& y) const {
        T d = 0;
        if (x > up_.x())
            d += (x - up_.x());
        else if (x < lo_.x())
            d += (lo_.x() - x);
        if (y > up_.y())
            d += (y - up_.y());
        else if (y < lo_.y())
            d += (lo_.y() - y);
        return d;
    }
    inline T udist(const T& x, const T& y) const {  // dist under UltraScale metric
        T d = 0;
        if (x > up_.x())
            d += 0.5 * (x - up_.x());
        else if (x < lo_.x())
            d += 0.5 * (lo_.x() - x);
        if (y > up_.y())
            d += (y - up_.y());
        else if (y < lo_.y())
            d += (lo_.y() - y);
        return d;
    }

    // TODO: use Point2 as para
    inline void set(const Point2<T>& p) {
        up_ = p;
        lo_ = p;
    }
    Box(const Point2<T>& p) { set(p); }
    inline void update(const Point2<T>& p) { update(p.x(), p.y()); }
    inline void fupdate(const Point2<T>& p) { fupdate(p.x(), p.y()); }
};

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const Box<T>& b) {
    os << "(" << b.up_ << ", " << b.lo_ << ")";
    return os;
}

template <typename T>
class DynamicBox1D {
private:
    multiset<T> data_;

public:
    inline void insert(const T& val) { data_.insert(val); }
    inline void erase(const T& val) {
        auto ret = data_.equal_range(val);
        data_.erase(ret.first);  // erase only one instance
    }
    inline size_t size() const { return data_.size(); }
    // lower/upper bound
    inline T l() const { return *data_.begin(); }
    inline T u() const { return *data_.rbegin(); }
    // second lower/upper bound
    inline T sl() const { return *(++data_.begin()); }
    inline T su() const { return *(++data_.rbegin()); }
    // others' lower/upper bound
    inline T ol(const T& val) const { return val != l() ? l() : sl(); }
    inline T ou(const T& val) const { return val != u() ? u() : su(); }
    // range
    inline T range() const { return u() - l(); }
    inline bool intersect(const T& left, const T& right) const { return !(right < l() || left > u()); }
    // print
    void print() const {
        for (auto e : data_) cout << e << " ";
        cout << endl;
    }
};

template <typename T>
class DynamicBox {
public:
    DynamicBox1D<T> x, y;
    inline void insert(const T& xv, const T& yv) {
        x.insert(xv);
        y.insert(yv);
    }
    inline void erase(const T& xv, const T& yv) {
        x.erase(xv);
        y.erase(yv);
    }
    inline size_t size() const { return x.size(); }
    inline T hp() const { return x.range() + y.range(); }
    inline T uhp() const { return 0.5 * x.range() + y.range(); }  // half perimeter under UltraScale metric
    void print() const {
        cout << "x:";
        x.print();
        cout << "y:";
        y.print();
    }
};

// unsorted, size of even number
template <typename T>
void GetMedianTwo(vector<T>& vals, T& l, T& h) {
    assert(vals.size() % 2 == 0);
    size_t m = vals.size() / 2;
    nth_element(vals.begin(), vals.begin() + m, vals.end());
    h = vals[m];
    l = *max_element(vals.begin(), vals.begin() + m);  // the last one is not included
}

// Note 1: Val should be light (basic data types or pointers)
// Note 2: Key can be pair<ScoreT1, ScoreT2>
template <typename Val, typename Key, typename Compare>
void ComputeAndSort(vector<Val>& values, function<Key(Val)> score, Compare comp, bool stable = false) {
    vector<pair<Key, Val>> kvs;
    kvs.reserve(values.size());
    for (auto& val : values) kvs.push_back({score(val), val});
    auto loc_comp = [&](const pair<Key, Val>& a, const pair<Key, Val>& b) { return comp(a.first, b.first); };
    if (stable)
        stable_sort(kvs.begin(), kvs.end(), loc_comp);
    else
        sort(kvs.begin(), kvs.end(), loc_comp);
    for (size_t i = 0; i < values.size(); ++i) {
        values[i] = kvs[i].second;
    }
}

template <class T>
void GetWinElem(vector<T>& candidates,
                vector<vector<T>>& map,
                pair<int, int> center,
                int winWidth,
                int hor_step = 2,
                int ver_step = 1) {
    int centerx = center.first;
    int centery = center.second;
    int lx = centerx - winWidth;
    int hx = centerx + winWidth;

    int nx = map.size();
    int ny = map[0].size();

    for (int x = lx, y = centery; x <= centerx; x += hor_step, y += ver_step) {
        if (x >= 0 && x < nx && y >= 0 && y < ny) {
            candidates.push_back(map[x][y]);
        }
    }
    for (int x = hx, y = centery; x > centerx; x -= hor_step, y -= ver_step) {
        if (x >= 0 && x < nx && y >= 0 && y < ny) {
            candidates.push_back(map[x][y]);
        }
    }
    for (int x = lx + hor_step, y = centery - ver_step; x <= centerx; x += hor_step, y -= ver_step) {
        if (x >= 0 && x < nx && y >= 0 && y < ny) {
            candidates.push_back(map[x][y]);
        }
    }
    for (int x = hx - hor_step, y = centery + ver_step; x > centerx; x -= hor_step, y += ver_step) {
        if (x >= 0 && x < nx && y >= 0 && y < ny) {
            candidates.push_back(map[x][y]);
        }
    }
}