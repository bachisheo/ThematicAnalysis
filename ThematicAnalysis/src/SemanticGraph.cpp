#include <execution>
#include <sstream>
#include <fstream>
#include <numeric>
#include "UGraphviz/UGraphviz.hpp"
#include "SemanticGraph.h"
#include "Utils/FileUtils.h"
#include "Hasher.h"
#include "Utils/StringUtils.h"

Node::Node(Term term) :
	term(term),
	weight(0)
{
}

Node::Node() : weight(0)
{
}

double Node::sumLinksWeight() const
{
	//if (isSumLinksWeightsChanged) {
	sumLinksWeights = 0;
	for (auto&& [hash, link] : neighbors)
	{
		sumLinksWeights += link.weight;
	}
	isSumLinksWeightsChanged = false;
	//	}
	return sumLinksWeights;

}

Link::Link(double weight)
	:weight(weight)
{
}

Link::Link() : Link(0)
{
}

size_t SemanticGraph::getNForNgram() const
{
	return _nForNgram;
}

SemanticGraph::SemanticGraph(size_t nForNgrams) : _nForNgram(nForNgrams)
{
}

void SemanticGraph::addTerm(Term const& term)
{
	nodes.emplace(term.getHashCode(), Node(term));
}

void SemanticGraph::createLink(size_t firstTermHash, size_t secondTermHash, double weight)
{
	auto& firstNode = nodes.at(firstTermHash);
	auto const& [_, isInserted] = firstNode.neighbors.emplace(secondTermHash, Link(weight));
	if (!isInserted) {
		if (isTermExist(firstTermHash) && isTermExist(secondTermHash)) {
			auto term1 = nodes.at(firstTermHash).term;
			auto term2 = nodes.at(secondTermHash).term;
			throw std::logic_error("Link " + term1.view + " -> " + term2.view + " already exist!");

		}
		throw std::logic_error("Link already exist!");
	}
}

void SemanticGraph::addTermWeight(size_t termHash, double weight)
{
	nodes.at(termHash).weight += weight;
}

double SemanticGraph::getLinkWeight(size_t firstTermHash, size_t secondTermHash) const
{
	return nodes.at(firstTermHash).neighbors.at(secondTermHash).weight;
}

bool SemanticGraph::isTermExist(size_t termHash) const
{
	return nodes.find(termHash) != nodes.end();
}

bool SemanticGraph::isLinkExist(size_t firstTermHash, size_t secondTermHash) const
{
	const auto& ptr = nodes.find(firstTermHash);
	return ptr != nodes.end() && ptr->second.neighbors.find(secondTermHash) != ptr->second.neighbors.end();
}


/**
 * \brief Extract subGraph
 * \param centerHash subGraph center term
 * \param radius extraction level (from center term)
 * \return result subGraph
 */
SemanticGraph SemanticGraph::getNeighborhood(size_t centerHash, unsigned radius, double minWeight) const
{
	auto neighbors = SemanticGraph();
	buildNeighborhood(centerHash, radius, minWeight, neighbors);
	return neighbors;
}

void SemanticGraph::buildNeighborhood(size_t curHash, unsigned radius, double minWeight,
	SemanticGraph& neighbors) const
{
	if (!isTermExist(curHash)) return;
	const auto& termNode = nodes.find(curHash)->second;
	neighbors.addTerm(termNode.term);
	if (radius == 0) return;
	for (auto&& [hash, link] : termNode.neighbors)
	{
		if (!neighbors.isTermExist(hash) && link.weight >= minWeight) {
			buildNeighborhood(hash, radius - 1, minWeight, neighbors);
			neighbors.createLink(curHash, hash, link.weight);
		}
	}
}

std::string doubleToString(double num)
{
	std::stringstream ss;
	ss << num;
	return ss.str();
}

std::string breakText(std::string const& text, size_t maxLen)
{
	auto words = StringUtils::split(text);
	std::stringstream ss;
	size_t len = 0;
	for (auto& word : words)
	{
		ss << word << ' ';
		len += word.size();
		if (len > maxLen)
		{
			//ss << '\n';
			len = 0;
		}
	}
	return ss.str();
}
Ubpa::UGraphviz::Graph SemanticGraph::createDotView(std::map<size_t, size_t>& registredNodes) const
{
	Ubpa::UGraphviz::Graph dotGraph("SemanticGraph", true);
	auto& reg = dotGraph.GetRegistry();
	for (auto&& [hash, node] : nodes)
	{
		auto nodeId = reg.RegisterNode(breakText(node.term.view, 6));
		dotGraph.AddNode(nodeId);
		registredNodes.emplace(hash, nodeId);
	}

	for (auto&& [hash, node] : nodes) {
		for (auto&& [neighbor_hash, link] : node.neighbors) {
			auto edgeId = reg.RegisterEdge(registredNodes[hash], registredNodes[neighbor_hash]);
			reg.RegisterEdgeAttr(edgeId, "label", doubleToString(link.weight));
			dotGraph.AddEdge(edgeId);
		}
	}
	dotGraph.RegisterGraphAttr("overlap", "false");
	return dotGraph;
}

std::string SemanticGraph::getDotView() const
{
	std::map<size_t, size_t> registredNodes;
	return createDotView(registredNodes).Dump();
}

std::string SemanticGraph::getDotView(size_t centerHash) const
{
	std::map<size_t, size_t> registredNodes;
	auto gr = createDotView(registredNodes);
	if (nodes.find(centerHash) != nodes.end())
	{
		gr.GetRegistry().RegisterNodeAttr(registredNodes[centerHash], "color", "red");
		gr.GetRegistry().RegisterNodeAttr(registredNodes[centerHash], "fontname", "times-bold");
	}
	return gr.Dump();
}


///	EXPORT/IMPORT FORMAT
/// <vertexes count>
/// <view>
/// <weight> <numberOfArticles> <normalized words count> <word 1> ...
/// ...
/// <edges count>
///	<first term index> <second term index> <weight>
///	...
void SemanticGraph::exportToFile(std::string const& filePath)
{
	std::ofstream fout(filePath);
	exportToStream(fout);
	fout.close();
}

void SemanticGraph::exportToStream(std::ostream& out)
{
	std::map<size_t, int> indexes;
	out << nodes.size() << std::endl;
	int index = 0;

	for (auto&& [hash, node] : nodes)
	{
		out << node.term.view << '\n' << node.weight << ' ' << node.term.numberOfArticlesThatUseIt << ' ' << node.term.normalizedWords.size() << ' ';
		for (auto&& word : node.term.normalizedWords)
			out << word << ' ';
		out << std::endl;
		indexes[hash] = index++;
	}

	auto edgesCount = std::transform_reduce(std::execution::par, nodes.begin(), nodes.end(), 0ull,
		[](size_t cnt, size_t cnt2) {return cnt + cnt2; },
		[](auto const& pair) {return pair.second.neighbors.size(); });

	out << edgesCount << std::endl;
	for (auto&& [hash, node] : nodes)
		for (auto&& [neighborHash, link] : node.neighbors)
			out << indexes[hash] << ' ' << indexes[neighborHash] << ' ' << link.weight << std::endl;
}

void SemanticGraph::importFromFile(std::string const& filePath)
{
	std::ifstream fin(filePath);
	std::stringstream ss(FileUtils::readAllFile(fin));
	importFromStream(ss);
	fin.close();
}

Term readTerm(std::istream& in)
{
	std::string view;
	double weight;
	size_t wordsCount, numberOfArticlesThatUseIt;
	std::ws(in);
	std::getline(in, view);
	in >> weight >> numberOfArticlesThatUseIt >> wordsCount;
	std::vector<std::string> words(wordsCount);
	for (size_t i = 0; i < wordsCount; i++)
		in >> words[i];

	Term term = { words, view, Hasher::sortAndCalcHash(words) };
	term.numberOfArticlesThatUseIt = numberOfArticlesThatUseIt;
	return term;
}

void SemanticGraph::importFromStream(std::istream& in)
{
	int termsCount, linksCount;
	in >> termsCount;
	std::vector<Term> terms;
	terms.reserve(termsCount);
	for (int i = 0; i < termsCount; i++) {
		auto term = readTerm(in);
		terms.push_back(term);
		addTerm(term);
	}
	in >> linksCount;
	while (linksCount--) {
		size_t firstTermIndex, secondTermIndex;
		double weight;
		in >> firstTermIndex >> secondTermIndex >> weight;

		createLink(terms[firstTermIndex].getHashCode(), terms[secondTermIndex].getHashCode(), weight);
	}
}

void drawDotToImage(std::string const& dotView, std::string const& dirPath, std::string const& imageName)
{
	std::string dotFile = "temp.dot";
	FileUtils::writeUTF8ToFile(dotFile, dotView);
	std::string command = std::string("external\\graphviz\\neato.exe  -Tpng temp.dot  -o ") + dirPath + imageName + ".png";
	system(command.c_str());
	std::remove(dotFile.c_str());
}

void SemanticGraph::drawToImage(std::string const& dirPath, std::string const& imageName) const
{
	drawDotToImage(getDotView(), dirPath, imageName);
}

void SemanticGraph::drawToImage(std::string const& dirPath, std::string const& imageName, size_t centerHash) const
{
	drawDotToImage(getDotView(centerHash), dirPath, imageName);
}