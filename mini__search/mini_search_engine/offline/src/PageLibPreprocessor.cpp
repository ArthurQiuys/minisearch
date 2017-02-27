 ///
 /// @file    PageLibPreprocessor.cpp
 /// @author  lemon(haohb13@gmail.com)
 /// @date    2016-01-19 11:35:20
 ///


#include "PageLibPreprocessor.hpp"
#include "GlobalDefine.hpp"

#include <stdio.h>
#include <time.h>
#include <fstream>
#include <sstream>

using std::ifstream;
using std::ofstream;
using std::stringstream;

namespace wd
{

PageLibPreprocessor::PageLibPreprocessor(Configuration & conf)
: _conf(conf)
{}



void PageLibPreprocessor::doProcess()
{
	readInfoFromFile();
	time_t t1 = time(NULL);
	cutRedundantPages();
	buildInvertIndexTable();
	time_t t2 = time(NULL);
	printf("preprocess time: %d min\n", (t2 - t1)/60);

	storeOnDisk();
	time_t t3 = time(NULL);
	printf("store time: %d min\n", (t3 - t2)/60);
}

void PageLibPreprocessor::readInfoFromFile()
{
	map<string, string> &configMap = _conf.getConfigMap();
	string pageLibPath = configMap[RIPEPAGELIB_KEY];
	string offsetLibPath = configMap[OFFSETLIB_KEY];

	ifstream pageIfs(pageLibPath.c_str());
	ifstream offsetIfs(offsetLibPath.c_str());

	if((!pageIfs.good()) || (!offsetIfs.good()))
	{
		cout << "page or offset lib open error" << endl;
	}

	string line;
	int docId, docOffset, docLen;

	while(getline(offsetIfs, line))//offsetIfs对应的文件中存储每篇文章的id+文章开始位置+文章长度
	{
		stringstream ss(line);
		ss >> docId >> docOffset >> docLen;//对应文件id,文件开始位置，文件长度
		
		string doc;
		doc.resize(docLen, ' ');//开辟docLen长度的空间（一定要先开空间，不然read不进去）
		pageIfs.seekg(docOffset, pageIfs.beg);//文件指针的位置现在移动到从文件头（pageIfs.beg）开始的后docOffset长度处
		pageIfs.read(&*doc.begin(), docLen);//从pageIfs的文件中写入到string类型的doc中
		//doc中存储着某篇文章(XML格式的id+url+title+content）
		WebPage webPage(doc, _conf, _jieba);
		_pageLib.push_back(webPage);
		//webPage中有vector<string> _topWords存放某篇文章频率前20的词语
		//map<string,int> _wordsMap;    //存储文档的所有词（去停用词之后）和该项词的词频

		_offsetLib.insert(std::make_pair(docId, std::make_pair(docOffset, docLen)));
	}
#if 0
	for(auto mit : _offsetLib)
	{
		cout << mit.first << "\t" << mit.second.first << "\t" << mit.second.second << endl;
	}
#endif
}

void PageLibPreprocessor::cutRedundantPages()
{
	for(size_t i = 0; i != _pageLib.size() - 1; ++i)
	{
		for(size_t j = i + 1; j != _pageLib.size(); ++j)
		{
			if(_pageLib[i] == _pageLib[j])
			{
				_pageLib[j] = _pageLib[_pageLib.size() - 1];
				//如果有两篇文章相同，则将最后一篇文章拷贝到相似文章的后一篇文章处，然后将最后一篇文章删除
				_pageLib.pop_back();
				--j;
			}
		}
	}
}

void PageLibPreprocessor::buildInvertIndexTable()
{
	for(auto page : _pageLib)
	{
		map<string, int> & wordsMap = page.getWordsMap();//存储文档中所有词（去停用词后）和该项词的词频
		for(auto wordFreq : wordsMap)
		{
			_invertIndexTable[wordFreq.first].push_back(std::make_pair(
					page.getDocId(), wordFreq.second));
		}
		//_invertIndexTable<词，vector<pair<DocId,词频>>
	}
	
	//计算每篇文档中的词的权重,并归一化
	map<int, double> weightSum;// 保存每一篇文档中所有词的权重平方和. int 代表docid

	int totalPageNum = _pageLib.size();
	for(auto & item : _invertIndexTable)
	{	
		int df = item.second.size();//vector的条数就是df:该词在多少篇文章中出现过
		//求关键词item.first的逆文档频率
		double idf = log(static_cast<double>(totalPageNum)/ df + 0.05) / log(2);//idf
		
		for(auto & sitem : item.second)
		{
			double weight = sitem.second * idf;	//w=tf*idf

			weightSum[sitem.first] += pow(weight, 2);
			sitem.second = weight;	
		}
	}

	for(auto & item : _invertIndexTable)
	{	//归一化处理
		for(auto & sitem : item.second)
		{
			sitem.second = sitem.second / sqrt(weightSum[sitem.first]);//归一化
		}
	}


#if 0 // for debug
	for(auto item : _invertIndexTable)
	{
		cout << item.first << "\t";
		for(auto sitem : item.second)
		{
			cout << sitem.first << "\t" << sitem.second <<  "\t";
		}
		cout << endl;
	}
#endif
}

void PageLibPreprocessor::storeOnDisk()
{
	sort(_pageLib.begin(), _pageLib.end());	

	ofstream ofsPageLib(_conf.getConfigMap()[NEWPAGELIB_KEY].c_str());
	ofstream ofsOffsetLib(_conf.getConfigMap()[NEWOFFSETLIB_KEY].c_str());

	if( !ofsPageLib.good() || !ofsOffsetLib.good())
	{	
		cout << "new page or offset lib ofstream open error!" << endl;
	}

	for(auto & page : _pageLib)
	{
		int id = page.getDocId();
		int length = page.getDoc().size();
		ofstream::pos_type offset = ofsPageLib.tellp();
		ofsPageLib << page.getDoc();//ofsPageLib中存储删除重复文档之后的文档xml格式：<docid+url+title+content>

		ofsOffsetLib << id << '\t' << offset << '\t' << length << '\n';//ofsOffsetLib中存储删除重复文档之后的文档在ofsPageLib中的<docid+文章起始位置+文档长度>
	}

	ofsPageLib.close();
	ofsOffsetLib.close();


	// invertIndexTable
	ofstream ofsInvertIndexTable(_conf.getConfigMap()[INVERTINDEX_KEY].c_str());
	if(!ofsInvertIndexTable.good())
	{
		cout << "invert index table ofstream open error!" << endl;
	}
	for(auto item : _invertIndexTable)
	{
		ofsInvertIndexTable << item.first << "\t";
		for(auto sitem : item.second)
		{
			ofsInvertIndexTable << sitem.first << "\t" << sitem.second <<  "\t";
		}
		ofsInvertIndexTable << endl;
	}
	ofsInvertIndexTable.close();
}

}// end of namespace wd
