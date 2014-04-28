#include <trilib/rbtree.h>
#include <psz/csv.h>

#include <gflags/gflags.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

DEFINE_string(data, "",
              "CSV file with data, columns should be (id, unique id, lat, "
              "long, name). id and unique id are optional and will be used "
              "only for verification purposes.");
DEFINE_double(radius, 0.0,
              "Radius that should be consider for searching similar objects.");
DEFINE_bool(print_debug, true, "");
DEFINE_bool(use_unique_id, false,
            "Use unique_id CSV column to verify accuracy.");

using uint32 = unsigned int;

class PointData {
 public:
  explicit PointData(uint32 id, uint32 unique_id, float lat, float lon,
                     string name)
      : id_(id),
        unique_id_(unique_id),
        latitude_(lat),
        longitude_(lon),
        name_(name) {}
  explicit PointData()
      : id_(0), unique_id_(0), latitude_(0.0f), longitude_(0.0f), name_() {}
  uint32 id_;
  uint32 unique_id_;
  float latitude_;
  float longitude_;
  string name_;
};

#include <ostream>
std::ostream& operator<<(std::ostream& ostr, const PointData& data) {
  ostr << data.id_ << "(" << data.unique_id_ << ") X (" << data.latitude_
       << ", " << data.longitude_ << ")";
  return ostr;
}

bool IsOrig(const PointData& data) { return data.id_ == data.unique_id_; }

class PointOrderByLat {
 public:
  bool operator()(const PointData& a, const PointData& b) const {
    return a.latitude_ < b.latitude_;
  }
};

class PointOrderByLong {
 public:
  bool operator()(const PointData& a, const PointData& b) const {
    return a.longitude_ < b.longitude_;
  }
};

template <typename T, typename CmpT>
class PointerCmp {
 public:
  PointerCmp() : value_cmp_() {}
  PointerCmp(const CmpT& cmp) : value_cmp_(cmp) {}
  bool operator()(T* a, T* b) const { return value_cmp_(*a, *b); }

 private:
  const CmpT value_cmp_;
};

inline float DistanceLat(const PointData& a, const PointData& b) {
  return std::abs(a.latitude_ - b.latitude_);
}

inline float DistanceLong(const PointData& a, const PointData& b) {
  return std::abs(a.longitude_ - b.longitude_);
}

inline float Distance(const PointData& a, const PointData& b) {
  return std::sqrt(std::pow(DistanceLat(a, b), 2.0f) +
                   std::pow(DistanceLong(a, b), 2.0f));
}

#define LOG_IF(condition) \
  if (FLAGS_print_debug && (condition)) std::cout
#define LOG \
  if (FLAGS_print_debug) std::cout

int true_positives = 0;
int duplicate_counter = 0;

void FindCloseByLongitude(
    float radius, const PointData& active_start,
    const trilib::RBTree<PointData*, PointerCmp<PointData, PointOrderByLong>>&
        active) {
  PointData by_long_start;
  PointData by_long_end;

  by_long_start.longitude_ = active_start.longitude_ - radius;
  by_long_end.longitude_ = active_start.longitude_ + radius;
  // find upper, lower bounds in rbtree
  auto iter_first =
      active.LowerBound(&by_long_start);  // iter_first should be always
                                          // non-empty because of a node itself
  auto iter_last = active.UpperBound(&by_long_end);
  if (iter_last != active.end()) {  // if not end, extend by 1
    ++iter_last;
  }
  int long_size = 0;
  while (iter_first != iter_last) {  // wrong condition
    ++long_size;
    bool is_self = &(active_start) == *iter_first;
    bool dist_close_enough =
        !is_self && Distance(active_start, **iter_first) < radius;
    if (dist_close_enough) {
      LOG << "\t\tconsidering duplicate: " << **iter_first << std::endl;
      ++duplicate_counter;
      if (FLAGS_use_unique_id &&
          (IsOrig(active_start) || IsOrig(**iter_first)) &&
          active_start.unique_id_ == (*iter_first)->unique_id_) {
        ++true_positives;
      }
    } else if (is_self) {
      LOG << "\t\tskipping self\n";
    } else {
      LOG << "\t\tskipping: " << **iter_first
          << Distance(active_start, **iter_first) << std::endl;
    }
    ++iter_first;
  }
  if (long_size > 0) {
    LOG << "\tbrowsed through " << long_size << " long elems\n";
  } else {
    LOG << "\tno items close enough\n";
  }
}

// taking addres of items in vector sounds like a bad idea (if vector
// reallocates -> huge problems)
void Cluster(float radius, vector<PointData>* points) {
  if (points->size() < 2) {
    return;
  }

  // First sort by Latitude.
  sort(points->begin(), points->end(), PointOrderByLat());

  // In tree we will keep all points closer than radius to active point, but
  // this time sorted by longitude.
  trilib::RBTree<PointData*, PointerCmp<PointData, PointOrderByLong>> active;
  vector<PointData>::iterator active_start, active_end;
  active_start = points->begin();
  active_end = active_start + 1;
  active.Insert(&(*active_start));
  LOG << "add initial " << *active_start << std::endl;
  float radius_2 = radius * radius;
  PointData by_long_start;
  PointData by_long_end;
  while (active_start != points->end() - 1) {
    LOG_IF(active_end != points->end())
        << "* " << *active_end << "  "
        << DistanceLat(*active_start, *active_end);
    if (active_start != active_end &&
        (active_end == points->end() ||
         DistanceLat(*active_start, *active_end) >= radius)) {
      // find all close
      LOG << " waiting \n\nactive for searching duplicates: " << *active_start
          << std::endl;
      FindCloseByLongitude(radius, *active_start, active);
      LOG << "\tdelete (self) " << *active_start << std::endl;
      active.Delete(&(*active_start));
      ++active_start;
    } else {
      LOG << " added" << std::endl;
      active.Insert(&(*active_end));
      ++active_end;
    }
  }
  std::cout << "\n\n";
  std::cout << "considered duplicates: " << duplicate_counter << std::endl;
  std::cout << "true_positives: " << true_positives << std::endl;
}

void LoadPoints(const std::string& filepath, std::vector<PointData>* data) {
  psz::CSVReader csv_reader(filepath);
  using std::strtol;
  using std::strtof;
  while (csv_reader.Next()) {
    const auto& row = csv_reader.Get();
    const auto id = strtol(row[0].c_str(), nullptr, 0);
    const auto uid = strtol(row[1].c_str(), nullptr, 0);
    const auto lat = strtof(row[2].c_str(), nullptr);
    const auto lon = strtof(row[3].c_str(), nullptr);
    data->emplace_back(PointData(id, uid, lat, lon, row[4]));
  }
}

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  vector<PointData> data;
  LoadPoints(FLAGS_data, &data);
  cout << "loaded " << data.size() << " entries\n";
  Cluster(FLAGS_radius, &data);
  return 0;
}
