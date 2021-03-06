#include "Boosting.h"

#include <iostream>

#include "Utils.h"
#include "ThreadManager.h"

Boosting::Boosting(
    int num_iterations,
    int max_leaves,
    double min_gamma,
    int max_trials_before_shrink,
    const std::vector<Bins>& bins,
    BufferLoader& training_loader,
    // serial_training_loader: SerialStorage,
    int max_sample_size,
    double default_gamma,
    std::shared_ptr<Model>& model
    ) : num_iterations(num_iterations),
    training_loader(training_loader),
    learner(max_leaves, min_gamma, default_gamma, max_trials_before_shrink, bins),
    model(model) {
    // add root node for balancing labels
    TreeScore base_tree_and_gamma = get_base_tree(max_sample_size, training_loader);
    Tree base_tree = base_tree_and_gamma.first;
    double gamma = base_tree_and_gamma.second;

    double gamma_squared = gamma * gamma;

    sum_gamma = gamma_squared;
    remote_sum_gamma = gamma_squared;

    model->push_back(base_tree);
}

/// Start training the boosting algorithm.
void Boosting::training(
    std::vector<Example> validate_set1,
    std::vector<Example> validate_set2
    ) {

    std::vector<double> validate_w1;
    std::vector<double> validate_w2;
    for (int i = 0; i < validate_set1.size(); ++i) {
        const Example& example = validate_set1[i];
        validate_w1.push_back(get_weight(example, (*model)[0].get_leaf_prediction(example)));
    }
    for (int i = 0; i < validate_set2.size(); ++i) {
        const Example& example = validate_set2[i];
        validate_w2.push_back(get_weight(example, (*model)[0].get_leaf_prediction(example)));
    }

    int iteration = 0;
    bool is_gamma_significant = true;


    while (is_gamma_significant && (iteration <= num_iterations || model->size() < num_iterations)) {

        std::vector<ExampleInSampleSet> data = training_loader.get_next_batch_and_update(true, model);
        //int batch_size = data.size();
        std::shared_ptr<Tree> new_rule = learner.update(data, validate_set1, validate_w1, validate_set2, validate_w2);

        if (new_rule) { // check if it is nonempty
            if (validate_set1.size() > 0) {

                for (int i = 0; i < validate_set1.size(); ++i) {
                    const Example& example = validate_set1[i];
                    validate_w1[i] *= get_weight(example, new_rule->get_leaf_prediction(example));
                }

                for (int i = 0; i < validate_set2.size(); ++i) {
                    const Example& example = validate_set2[i];
                    validate_w2[i] *= get_weight(example, new_rule->get_leaf_prediction(example));
                }
            }

            model->push_back(*new_rule);
            is_gamma_significant = learner.is_gamma_significant();

            if (is_gamma_significant) {
                learner.reset_all();
            }
        }
        ++iteration;
        std::cout << "iteration " << iteration << " completed. Model length " << model->size() << std::endl;
    }

    std::cout << "Training is finished. Model length: " << model->size() << ". Is gamma significant? " <<
        learner.is_gamma_significant()  << std::endl;
    ThreadManager::continue_run = false;
}
