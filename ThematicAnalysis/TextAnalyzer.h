#pragma once
#include "SemanticGraph.h"

class TextAnalyzer
{
public:
	//why public?
	SemanticGraph tagsGraph;
	void analyze(std::string textFilePath, SemanticGraph graph);
	void analyze(std::vector<std::string> normalizedText, SemanticGraph graph);
	std::vector<std::string> getRelevantTags(int tagsCount);
private:
	void rebuildTagsGraph(size_t termHash);
	void recalculateTermWeight(size_t centerTermHash, size_t radius, double delta = 1.);
	void mergeGraphs(SemanticGraph src);
	static const double WEIGHT_ADDITION;
	static const size_t LINK_RADIUS;
	SemanticGraph _sourceGraph;
};


