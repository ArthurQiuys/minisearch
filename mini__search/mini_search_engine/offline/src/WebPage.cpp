 ///
 /// @file    WebPage.cpp
 /// @author  lemon(haohb13@gmail.com)
 /// @date    2016-01-18 17:28:29
 ///

#include "WebPage.hpp"

#include <stdlib.h>
#include <iostream>
#include <queue>
#include <algorithm>

using std::cout;
using std::endl;
using std::priority_queue;
using std::pair;
using std::make_pair;


namespace wd
{

struct WordFreqCompare
{
	bool operator()(const pair<string, int> & left, const pair<string, int> & right)//如果left.second小则返回真，否则返回假，所以.second大的优先级高，优先级高的排在前面
	{
		if(left.second < right.second)
		{	return true;	}
		else if(left.second == right.second && left.first > right.first)//运算符重载
		{	return true;	}
		else
		{	return false;	}
	}
};


WebPage::WebPage(string & doc, Configuration & config, WordSegmentation & jieba)
: _doc(doc)
{
	//cout << "WebPage()" << endl;
	_topWords.reserve(20);
	processDoc(doc, config, jieba);

}


void WebPage::processDoc(const string & doc, Configuration & config, WordSegmentation & jieba)
{
	string docIdHead = "<docid>";
	string docIdTail = "</docid>";
	string docUrlHead = "<docurl>";
	string docUrlTail = "</docurl>";
	string docTitleHead = "<doctitle>\n";
	string docTitleTail = "\r\n</doctitle>";
	string docContentHead = "<doccontent>\n";
	string docContentTail = "\n</doccontent>";

	//提取文档的docid
	int bpos = doc.find(docIdHead);
	int epos = doc.find(docIdTail);
	string docId = doc.substr(bpos + docIdHead.size(), 
					epos - bpos - docIdHead.size());
	_docId = atoi(docId.c_str());

	//title
	bpos = doc.find(docTitleHead);
	epos = doc.find(docTitleTail);
	_docTitle = doc.substr(bpos + docTitleHead.size(), 
					epos - bpos - docTitleHead.size());

	//cout << "========" << endl << _docTitle << endl;
	//content
	bpos = doc.find(docContentHead);
	epos = doc.find(docContentTail);
	_docContent = doc.substr(bpos + docContentHead.size(),
					epos - bpos - docContentHead.size());

	//cout << "========" << endl << _docContent << endl;


	//分词
	vector<string> wordsVec = jieba(_docContent.c_str());//WordSegmentation中重载了()
	//jieba(_docContent.c_str())将_docContent.c_str()分词，并返回vector<string>类型的分词结果
	set<string> & stopWordList = config.getStopWordList();//config.getStopWordList()返回set<string>类型的停用词表
	calcTopK(wordsVec, TOPK_NUMBER, stopWordList);

}


void WebPage::calcTopK(vector<string> & wordsVec, int k, set<string> & stopWordList)
{
	for(auto iter = wordsVec.begin(); iter != wordsVec.end(); ++iter)
	{
		auto sit = stopWordList.find(*iter);//在stopWordList中查找iter，如果没有找到，则sit偏移到stopWordList.end()
		if(sit == stopWordList.end())
		{
			++_wordsMap[*iter];//_wordsMap中存储文档的所有词和词频<词，词频>（去停用词后的）
		}
	}

	
	priority_queue<pair<string, int>, vector<pair<string, int> >, WordFreqCompare>//WordFreqCompare比较策略
		wordFreqQue(_wordsMap.begin(), _wordsMap.end());

	while(!wordFreqQue.empty())
	{
		string top = wordFreqQue.top().first;
		wordFreqQue.pop();
		if(top.size() == 1 && (static_cast<unsigned int>(top[0]) == 10 ||
			static_cast<unsigned int>(top[0]) == 13))
		{	continue;	}//去除个别特殊的频率特别高的字符

		_topWords.push_back(top);
		if(_topWords.size() >= TOPK_NUMBER)//TOPK_NUMBER=20，_topWords中只存储前频率前20的词语
		{
			break;
		}
	}


#if 0
	for(auto mit : _wordsMap)
	{
		cout << mit.first << "\t" << mit.second << std::endl;	
	}
	cout << endl;

	for(auto word : _topWords)
	{
		cout << word << "\t" << word.size() << "\t" << static_cast<unsigned int>(word[0]) << std::endl;
	}
#endif
}

// 判断两篇文档是否相同
bool operator == (const WebPage & lhs, const WebPage & rhs) 
{
	int commNum = 0;
	auto lIter = lhs._topWords.begin();
	for(;lIter != lhs._topWords.end(); ++lIter)
	{
		commNum += std::count(rhs._topWords.begin(), rhs._topWords.end(), *lIter);//从开头到结尾与lIter相同的字符的个数（topWords都是不同的）
	}
	
	int lhsNum = lhs._topWords.size();
	int rhsNum = rhs._topWords.size();
	int totalNum = lhsNum < rhsNum ? lhsNum : rhsNum;

	if( static_cast<double>(commNum) / totalNum > 0.75 )
	{	return true;	}
	else 
	{	return false;	}
}

//对文档按照docId进行排序
bool operator < (const WebPage & lhs, const WebPage & rhs)
{
	if(lhs._docId < rhs._docId)
		return true;
	else 
		return false;
}

}// end of namespace wd
