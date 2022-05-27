﻿#include <clocale>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>
#include <boost/regex.hpp>

#include "ArticlesNormalizer.h"
#include "FileManager.h"
#include "Hasher.h"
#include "TextNormalizer.h"
#include "SemanticGraphBuilder.h"
#include "TextAnalyzer.h"
#include "ArticlesReader/MathArticlesReader.h"


void create() {
	auto builder = SemanticGraphBuilder();
	auto graph = builder.build("resources/math/math.txt", MathArticlesReader());
	graph.exportToFile("resources/coolAllMath.gr");
}

SemanticGraph getMathGraph()
{
	SemanticGraph graph;
	graph.importFromFile("resources/coolAllMath.gr");
	return graph;
}

void draw()
{
	auto graph = getMathGraph();
	TextNormalizer normalizer;
	auto hash = Hasher::sortAndCalcHash(normalizer.normalize("бэра классы"));
	auto subgr = graph.getNeighborhood(hash, 1, 0.05);
	subgr.drawToImage("", "image", hash);
}

void tags()
{
	auto graph = getMathGraph();

	TextAnalyzer analyzer;

	analyzer.analyze(FileManager::readAllFile("resources/voevoda.txt"), graph);
	auto tags = analyzer.getRelevantTags(100);
	for (auto& tag : tags)
	{
		//std::cout << tag << '\n';
	}
}


int main() {
	setlocale(LC_ALL, "rus");
	//create();
	tags();
	return 0;
}
