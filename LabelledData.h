#pragma once

#include <vector>

// Training example. It consists of two fields: `feature` and `label`.
template<class TFeature, class TLabel>
struct LabeledData {

    LabeledData(const std::vector<TFeature>& feature, TLabel label): feature(feature), label(label) {
    }

    std::vector<TFeature> feature;
    TLabel label;
};

typedef double RawTFeature;
typedef int TFeature;
typedef int TLabel;

typedef LabeledData<RawTFeature, TLabel> RawExample;
typedef LabeledData<TFeature, TLabel> Example;

typedef std::pair<Example, std::pair<double, int>> ExampleInSampleSet;
typedef std::pair<Example, std::pair<double, int>> ExampleWithScore;