#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <armadillo>

using namespace rapidjson;

int main(int argc, char * argv[])
{

  int index = std::stoi(argv[2]);
  std::map<std::string,std::pair<float,float>> plot_centers;

  std::unordered_map<std::string,int> all_words;
  std::ifstream ifs(argv[1]);
  std::vector<std::string> files;
  std::vector<std::unordered_map<std::string,double> > docs;

  for (std::string line; std::getline(ifs, line); ) 
    files.emplace_back(line);

  size_t doc_index=0;

  for (auto &x : files)
  {

    std::ifstream ifs(x);
    std::string model;
    for (std::string line; std::getline(ifs, line); ) 
      model+=line;

    Document d;
    if(!d.Parse(model.c_str()).HasParseError())
    {
      const rapidjson::Value& t = d["terms"];
      const rapidjson::Value& f = d["frequencies"];

      std::unordered_map<std::string,double> keywords;

      auto terms = t.GetArray();
      auto freqs = f.GetArray();

      for (int i=0; i<terms.Size(); i++)
      {
        std::string key = terms[i].GetString();
        double v = static_cast<double>(freqs[i].GetInt());
        keywords[key]=v;

        all_words[key]+=freqs[i].GetInt();
      }

      docs.emplace_back(keywords);
    }
    else
    {
      std::cerr << "parse error: " << x << std::endl;
    }

    if (d.HasMember("linplots"))
    {
      auto p = d["linplots"].GetArray();

      for (auto &v : p)
      {

        if (v.HasMember("gmm"))
        {
          const rapidjson::Value& g = v["gmm"];

          if (g.IsArray())
          {

          }
          else
          {
            
            std::string page = g["page"].GetString();
            if (g["models"].IsArray())
            {
              auto data = g["models"].GetArray();
              for (auto &w : data)
              {

                auto xx = w["x"].GetArray();
                auto yy = w["y"].GetArray();

                float xa=0.;
                float ya=0.;

                for (int i=0; i<xx.Size(); i++)
                {
                  xa+=xx[i].GetFloat();
                  ya+=yy[i].GetFloat();
                }
                xa/=(float)xx.Size();
                ya/=(float)yy.Size();

                std::string label=std::to_string(doc_index)+"."+page;
                plot_centers[label]=std::pair<float,float>{xa,ya};
              }
            }
            else
            {

            }
          }
        }
      }
    }

    doc_index++;
  }

   std::vector<std::pair<int,std::string>> words;
    for (auto &x : all_words)
      words.emplace_back(std::pair<int,std::string>{x.second, x.first});

    std::sort(words.begin(),words.end());

    //for (auto &x : words)
    //std::cerr << x.first << " " << x.second << std::endl;
    
    words.clear();

    auto source = docs.at(index);

    arma::mat adj, euc1;
    adj.resize(docs.size(),docs.size());
    euc1.resize(docs.size(),docs.size());

    for (auto &x : source)
      words.emplace_back(std::pair<int,std::string>{x.second, x.first});

    std::sort(words.begin(),words.end());

    std::string output1 = "[";
    for (auto &x : words)
      output1+= "\"" + x.second + "\",";

    output1[output1.size()-1]=']';

    std::cerr << output1 << std::endl;

    for (int i=0; i<docs.size(); i++)
    {
      auto ti = docs.at(i);
      for (int j=i+1; j<docs.size(); j++) 
      {
        auto tj = docs.at(j);
        float v = 0;
        for (auto &x : ti)
        {
          auto it = tj.find(x.first);
          v += (it!=tj.end()) ? 1. : 0.;
        }
        adj(i,j)=v;
      }
    }

    adj+=adj.t();
    adj.save("adj.txt",arma::csv_ascii);

    for (int i=0; i<docs.size(); i++)
    {
      auto ti = docs.at(i);

      for (int j=i+1; j<docs.size(); j++) 
      {
        auto tj = docs.at(j);
        double d=0.;
        for (auto &x : ti)
        {
          auto it = tj.find(x.first);
          auto u = (it!=tj.end()) ? it->second : 0.;

          d+=(u-x.second)*(u-x.second);

        }
        euc1(i,j)=(d>800) ? 0. : d;
      }
    }


    euc1+=euc1.t();
    euc1.save("euc1.txt",arma::csv_ascii);

    std::string output2="{ \"nodes\": ["; 


    for (int i=0; i<files.size(); i++)
    {
      output2+="{\"id\":"+std::to_string(i); 
      output2+=",\"group\":1},";
    }

    output2[output2.size()-1]=']';
    output2+=",\"links\": [";

    for (int i=0; i<docs.size(); i++)
      for (int j=i+1; j<docs.size(); j++)
        if (euc1(i,j)>0)
        {
          output2+="{\"source\":"+std::to_string(i);
          output2+=",\"target\":"+std::to_string(j);
          output2+=",\"value\":"+std::to_string((int)euc1(i,j));
          output2+="},";
        }

    output2[output2.size()-1]=']';
    output2+="}";

    std::cerr << output2 << std::endl;
    std::string output3="{ \"nodes\": ["; 

    for (auto &x : plot_centers)
    {
      output3+="{\"id\":\""+x.first; 
      output3+="\",\"group\":1},";
    }

    output3[output3.size()-1]=']';
    output3+=",\"links\": [";

    for (auto &x : plot_centers)
      for (auto &y : plot_centers)
      {
        if (x.first.find(y.first)!=std::string::npos) continue;

        float dist = (x.second.first-y.second.first) * (x.second.first-y.second.first) \
         + (x.second.second-y.second.second) * (x.second.second-y.second.second);
        if (dist< 5. && dist > 0.1)
        {
          output3+="{\"source\":\""+x.first;
          output3+="\",\"target\":\""+y.first;
          output3+="\",\"value\":"+std::to_string((int)dist);
          output3+="},";
        }
      }

    output3[output3.size()-1]=']';
    output3+="}";

    std::cerr << output3 << std::endl;
    return 0;
}
