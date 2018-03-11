#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <math.h>
#include <vector>
#include <limits.h>
#include <dirent.h>
using namespace std;

// read result file generated from kernel module.
// The file *must* have as first line # number_records
// Each column *must* have number_records elements.

int check_file(ofstream * file, string name){
  if(!(*file).is_open()){
    cout << "Unable to open " << name << endl;
    return -1;
  }
  return 0;
}

int check_file(ifstream * file, string name){
  if(!(*file).is_open()){
    cout << "Unable to open " << name << endl;
    return -1;
  }
  return 0;
}

void open_dir(const char * directory, vector<string> * out){
  DIR *dir;
  struct dirent *ent;
  string s = directory;
  if ((dir = opendir (directory)) != NULL) {
    while ((ent = readdir (dir)) != NULL) {
      // skip .files and _files
      if(ent->d_name[0] != '.' && ent->d_name[0] != '_' ){
        (*out).push_back(s + "/" + ent->d_name);
      }
    }
    closedir (dir);
  } else {
    perror ("Failed to open directory");
    exit(0);
  }
}

void calculate_stats(unsigned long * res, vector<unsigned long> troughput, int trough_count, vector<unsigned long> latency, int lat_count ){
  unsigned long t_median = troughput[trough_count/2];
  unsigned long l_median = latency[lat_count/2];
  cout << "\ttroughput median: " << t_median << endl;
  cout << "\tlatency median: " << l_median << endl;

  unsigned long t_99perc = troughput[trough_count * 99 / 100];
  unsigned long l_99perc = latency[lat_count * 99 / 100];
  cout << "\ttroughput 99 percentile: " << t_99perc << endl;
  cout << "\tlatency 99 percentile: " << l_99perc << endl;

  res[1] = t_median;
  res[2] = l_median;
  res[3] = t_99perc;
  res[4] = l_99perc;
}


int read_from_file(string filename, unsigned long * res){
  string line;
  const char * str = filename.c_str();
  ifstream myfile(str);
  int nrecords = 0, nclients = 0;

  if(check_file(&myfile, filename))
    return -1;
  // read size of file
  getline (myfile,line);
  stringstream ss(line);
  char s;
  ss >> s >> nrecords >> nclients;
  if(nrecords <= 0 || nclients <= 0){
    cout << "Wrong file format for " << filename << "\n" << endl;
    return -1;
  }
  vector<unsigned long> troughput(nrecords,0);
  vector<unsigned long> latency(nrecords,0);
  int trough_count=0, lat_count = 0;

  while (getline (myfile,line)){
    if(line[0] == '#' || line[0] == '\n')
      continue;
    stringstream ss(line);
    unsigned long t,l;
    ss >> t >> l;
    troughput[trough_count++] = t;
    if(l > 0)
      latency[lat_count++] = l;

    // cout << troughput[trough_count-1] << " " << latency[lat_count-1] << endl;
  }
  myfile.close();
  sort(troughput.begin(), troughput.end());
  sort(latency.begin(), latency.end());

  cout << filename << ": " << endl;
  cout << "\tN records: " << nrecords << endl;

  res[0] = nclients;
  calculate_stats(res, troughput, trough_count, latency, lat_count);
  cout << endl;

  return 0;
}

// input: folder with all data, filename of the resulting file to append
int main(int argc, char const *argv[]) {
  if(argc < 3){
    cout << "Usage: " << argv[0] << " folder output_filename\n";
    return 0;
  }
  double bar_width = 0.90;

  ofstream outfile(argv[2], ios_base::trunc);
  check_file(&outfile, argv[2]);

  vector<string> files;
  open_dir(argv[1], &files);
  sort(files.begin(), files.end());

  outfile << "#nclients\ttmedian\tlmedian\tt99\tl99" << "\n";
  outfile << "\"Results\"" << "\n";
  vector<unsigned long> latencies;

  for (size_t i = 0; i < files.size(); i++) {
    unsigned long stats[5];
    if(read_from_file(files[i], stats) >= 0){
      latencies.push_back(stats[2]/1000);
      outfile << stats[0] << "\t" << stats[1]*10 << "\t" << stats[2]/1000 << "\t" << stats[3]*10 << "\t" << stats[4]/1000 << "\t" << bar_width << "\n";
    }
  }


  sort(latencies.begin(), latencies.end());

  int pos_arr = 0;
  vector< pair<unsigned long, double> > v;
  v.push_back(make_pair(latencies[0], 1));

  for (size_t i = 1; i < latencies.size(); i++) {
    if(latencies[i] == v[pos_arr].first){
      v[pos_arr].second++;
    }else{
      v.push_back(make_pair(latencies[i], 1));
      pos_arr++;
    }
  }

  cout << "CDF:\n";
  outfile << "\n\n\"CDF\"\n";

  for (size_t i = 0; i < v.size(); i++) {
    v[i].second = v[i].second / latencies.size() * 100;
    cout << v[i].first << "\t" << v[i].second << "\n";

    if(i == 0)
      outfile << v[i].first << "\t" << v[i].second << "\n";
    else{
      outfile << v[i].first << "\t" << v[i-1].second << "\n";
      v[i].second += v[i-1].second;
      outfile << v[i].first << "\t" << v[i].second << "\n";
    }

  }

  outfile.close();

}
