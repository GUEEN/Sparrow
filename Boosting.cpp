#include <iostream>

#include "Boosting.h"
#include "Model.h"


Boosting::Boosting(
    int num_iterations,
    int max_leaves,
    double min_gamma,
    int max_trials_before_shrink,
    BufferLoader training_loader,
    // serial_training_loader: SerialStorage,
    std::vector<Bins> bins,
    Range range,
    int max_sample_size,
    double default_gamma
    ) : num_iterations(num_iterations), learner(learner), model(model),    
    learner(max_leaves, min_gamma, default_gamma, max_trials_before_shrink, bins, range) {
    //let mut training_loader = training_loader;

    // add root node for balancing labels
    auto base_tree_and_gamma = get_base_tree(max_sample_size, training_loader);

    Tree base_tree = base_tree_and_gamma.first;
    double gamma = base_tree_and_gamma.second;
   
    double gamma_squared = gamma ** gamma;

    sum_gamma = gamma_squared;
    remote_sum_gamma = gamma_squared;

    model.push_back(base_tree);

    //BufWriter persist_file_buffer(&String::from("model.json")));
}

/// Start training the boosting algorithm.
void Boosting::training(
    std::vector<Example> validate_set1,
    std::vector<Example> validate_set2
    ) {
    std::cout << "Start training" << std::endl;

    std::vector<double> validate_w1;
    std::vector<double> validate_w2;
    for (int i = 0; i < validate_set1.size(); ++i) {
        const Example& example = validate_set1[i];
        validate_w1.push_back(get_weight(example, model[0].get_leaf_prediction(example)));
    }
    for (int i = 0; i < validate_set2.size(); ++i) {
        const Example& example = validate_set1[i];
        validate_w2.push_back(get_weight(example, model[0].get_leaf_prediction(example)));
    }
    
    int iteration = 0;
    bool is_gamma_significant = true;

    while (is_gamma_significant && (num_iterations <= 0 || model.size() < num_iterations)) {

        auto data = training_loader.get_next_batch_and_update(true, model);
        int batch_size = data.size();
        Tree new_rule = learner.update(data, &validate_set1, &validate_w1, &validate_set2, &validate_w2);
 
        if (new_rule.is_some()) { // check if it is nonempty
            if (validate_set1.size() > 0) {

                for (int i = 0; i < validate_set1.size(); ++i) {
                    const Example& example = validate_set1[i];
                    validate_w1[i] *= get_weight(example, new_rule.get_leaf_prediction(example));
                }

                for (int i = 0; i < validate_set2.size(); ++i) {
                    const Example& example = validate_set2[i];
                    validate_w2[i] *= get_weight(example, new_rule.get_leaf_prediction(example));
                }
            }

            model.push_back(new_rule);

            is_gamma_significant = learner.is_gamma_significant();
            learner.reset_all();
        }
        ++iteration;
    }

    std::cout << "Training is finished. Model length: " << model.size() << ". Is gamma significant?" <<
        learner.is_gamma_signinificant() << ".\n";
}
