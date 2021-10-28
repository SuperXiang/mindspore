/**
 * Copyright 2021 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "runtime/framework/actor/actor_dump.h"

namespace mindspore {
namespace runtime {
namespace {
std::string GetSplitName(const std::string &name) {
  auto index = name.rfind('/');
  if ((index != std::string::npos) && (index < name.size() - 1)) {
    return name.substr(index + 1);
  }
  return name;
}

void DumpAbstractActor(const AbstractActor *actor, std::ofstream &ofs) {
  MS_EXCEPTION_IF_NULL(actor);
  if (actor->device_contexts().size() > 0) {
    ofs << "\t\tdevice_contexts:" << actor->device_contexts().size() << "\n ";
    for (const auto &device_context : actor->device_contexts()) {
      if (device_context == nullptr) {
        ofs << "\t\t\tdevice_context:" << device_context << "\n";
        continue;
      }
      ofs << "\t\t\tdevice_context:" << device_context->device_context_key().ToString() << "\n";
    }
  }

  if (actor->device_tensor_store_keys().size() > 0) {
    ofs << "\t\tdevice_tensor_store_keys:" << actor->device_tensor_store_keys().size() << "\n ";
    for (const auto &device_tensor_store_key : actor->device_tensor_store_keys()) {
      MS_EXCEPTION_IF_NULL(device_tensor_store_key.second);
      ofs << "\t\t\tto_input_index:" << device_tensor_store_key.first
          << "\tfrom_node_name:" << device_tensor_store_key.second->fullname_with_scope() << "\n";
    }
  }

  if (actor->input_data_arrow_aids().size() > 0) {
    ofs << "\t\tinput_data_arrow_actors:" << actor->input_data_arrow_aids().size() << "\n ";
    for (const auto &input_data_arrow_aid : actor->input_data_arrow_aids()) {
      ofs << "\t\t\tfrom_actor_name:" << input_data_arrow_aid.Name() << "\n";
    }
  }

  if (actor->input_control_arrow_aids().size() > 0) {
    ofs << "\t\tinput_control_arrow_actors:" << actor->input_control_arrow_aids().size() << "\n ";
    for (const auto &input_control_arrow_aid : actor->input_control_arrow_aids()) {
      ofs << "\t\t\tfrom_actor_name:" << input_control_arrow_aid.Name() << "\n";
    }
  }

  if (actor->output_data_arrows().size() != actor->output_data_nodes().size()) {
    MS_LOG(EXCEPTION) << "The size of output data arrows is not equal to the output nodes.";
  }
  if (actor->output_data_arrows().size() > 0) {
    ofs << "\t\toutput_data_arrows:" << actor->output_data_arrows().size() << "\n ";
    for (size_t i = 0; i < actor->output_data_arrows().size(); ++i) {
      auto data_arrow = actor->output_data_arrows()[i];
      auto output_node = actor->output_data_nodes()[i];
      MS_EXCEPTION_IF_NULL(data_arrow);
      std::string node_name = (output_node != nullptr) ? GetSplitName(output_node->fullname_with_scope()) : "";
      ofs << "\t\t\tfrom_output_node:" << node_name << "\tfrom_output_index:" << data_arrow->from_output_index_
          << "\tto_actor_name:" << data_arrow->to_op_id_.Name() << "\tto_input_index:" << data_arrow->to_input_index_
          << "\n";
    }
  }

  const auto &output_control_arrows = actor->output_control_arrows();
  if (output_control_arrows.size() > 0) {
    ofs << "\t\toutput_control_arrows:" << output_control_arrows.size() << "\n ";
    for (const auto &aid : output_control_arrows) {
      ofs << "\t\t\tto_actor_name:" << aid.Name() << "\n";
    }
  }

  if (actor->output_result_arrows().size() != actor->output_result_nodes().size()) {
    MS_LOG(EXCEPTION) << "The size of output result arrows is not equal to the output nodes.";
  }
  if (actor->output_result_arrows().size() > 0) {
    ofs << "\t\toutput_result_arrows:" << actor->output_result_arrows().size() << "\n ";
    for (size_t i = 0; i < actor->output_result_arrows().size(); ++i) {
      auto result_arrow = actor->output_result_arrows()[i];
      auto output_node = actor->output_result_nodes()[i];
      MS_EXCEPTION_IF_NULL(result_arrow);
      MS_EXCEPTION_IF_NULL(output_node);
      ofs << "\t\t\tfrom_output_node:" << GetSplitName(output_node->fullname_with_scope())
          << "\tfrom_output_index:" << result_arrow->from_output_index_
          << "\tto_actor_name:" << result_arrow->to_op_id_.Name()
          << "\toutput_node_position:" << result_arrow->to_input_index_ << "\n";
    }
  }
}

void DumpDSActor(const DataSourceActor *actor, std::ofstream &ofs) {
  MS_EXCEPTION_IF_NULL(actor);
  const auto &actor_name = actor->GetAID().Name();
  ofs << "\tactor_name:" << actor_name << "\n";

  if (actor->type() == KernelTransformType::kDeviceDataSourceActor) {
    // Dump the member info of device queue data source actor.
    const auto &device_queue_ds_actor = dynamic_cast<const DeviceQueueDataSourceActor *>(actor);
    MS_EXCEPTION_IF_NULL(device_queue_ds_actor);
    const auto &data_kernel = device_queue_ds_actor->data_kernel();
    MS_EXCEPTION_IF_NULL(data_kernel);
    ofs << "\t\tdata_kernel_name:" << data_kernel->fullname_with_scope()
        << "\tinput_number:" << AnfAlgo::GetInputTensorNum(data_kernel)
        << "\toutput_number:" << AnfAlgo::GetOutputTensorNum(data_kernel) << "\n";
    for (size_t i = 0; i < AnfAlgo::GetOutputTensorNum(data_kernel); ++i) {
      const auto &device_tensor = AnfAlgo::GetMutableOutputAddr(data_kernel, i, false);
      MS_EXCEPTION_IF_NULL(device_tensor);
      ofs << "\t\t\toutput_index:" << i << "\tptr:" << device_tensor->GetPtr() << "\tsize:" << device_tensor->GetSize()
          << "\toriginal_ref_count:" << device_tensor->original_ref_count() << "\n ";
    }
  } else if (actor->type() == KernelTransformType::kHostDataSourceActor) {
    // Dump the member info of host queue data source actor.
    const auto &host_queue_ds_actor = dynamic_cast<const HostQueueDataSourceActor *>(actor);
    MS_EXCEPTION_IF_NULL(host_queue_ds_actor);
    ofs << "\t\tdata_nodes:" << host_queue_ds_actor->data_nodes().size() << "\n";
    for (size_t i = 0; i < host_queue_ds_actor->data_nodes().size(); ++i) {
      const auto &data_node = host_queue_ds_actor->data_nodes()[i];
      MS_EXCEPTION_IF_NULL(data_node);
      const auto &device_tensor = AnfAlgo::GetMutableOutputAddr(data_node, 0, false);
      MS_EXCEPTION_IF_NULL(device_tensor);
      ofs << "\t\t\tnode_order_number:" << i << "\tnode_name:" << data_node->fullname_with_scope()
          << "\tptr:" << device_tensor->GetPtr() << "\tsize:" << device_tensor->GetSize()
          << "\toriginal_ref_count:" << device_tensor->original_ref_count() << "\n";
    }
  }

  DumpAbstractActor(actor, ofs);
  ofs << "\n";
}

void DumpKernelActor(const KernelActor *actor, std::ofstream &ofs) {
  MS_EXCEPTION_IF_NULL(actor);
  ofs << "\tactor_name:" << actor->GetAID().Name() << "\n";

  const auto &kernel = actor->kernel();
  MS_EXCEPTION_IF_NULL(kernel);
  ofs << "\t\tkernel_name:" << kernel->fullname_with_scope() << "\tinputs_num:" << AnfAlgo::GetInputTensorNum(kernel)
      << "\toutputs_num:" << AnfAlgo::GetOutputTensorNum(kernel) << "\n";
  for (size_t i = 0; i < AnfAlgo::GetOutputTensorNum(kernel); ++i) {
    const auto &device_tensor = AnfAlgo::GetMutableOutputAddr(kernel, i, false);
    MS_EXCEPTION_IF_NULL(device_tensor);
    ofs << "\t\t\toutput_index:" << i << "\tptr:" << device_tensor->GetPtr() << "\tsize:" << device_tensor->GetSize()
        << "\toriginal_ref_count:" << device_tensor->original_ref_count() << "\n ";
  }

  DumpAbstractActor(actor, ofs);
  ofs << "\n";
}

void DumpSuperKernelActor(const SuperKernelActor *actor, std::ofstream &ofs) {
  MS_EXCEPTION_IF_NULL(actor);
  ofs << "\tactor_name:" << actor->GetAID().Name() << "\n";

  const auto &graph = actor->graph();
  MS_EXCEPTION_IF_NULL(graph);

  ofs << "\t\tgraph_id:" << graph->graph_id() << "\tgraphl_name:" << graph->ToString()
      << "\tis_executing_sink:" << graph->is_executing_sink() << "\tis_loop_count_sink:" << graph->is_loop_count_sink()
      << "\tinputs_num:" << (graph->input_nodes()).size() << "\tkernels_num:" << (graph->execution_order()).size()
      << "\n";

  DumpAbstractActor(actor, ofs);
  ofs << "\n";
}

void DumpCopyActor(const CopyActor *actor, std::ofstream &ofs) {
  MS_EXCEPTION_IF_NULL(actor);
  ofs << "\tactor_name:" << actor->GetAID().Name() << "\n";

  auto device_tensor = actor->output();
  if (device_tensor != nullptr) {
    ofs << "\t\toutput_index:" << 0 << "\tptr:" << device_tensor->GetPtr() << "\tsize:" << device_tensor->GetSize()
        << "\toriginal_ref_count:" << device_tensor->original_ref_count() << "\n ";
  }

  DumpAbstractActor(actor, ofs);
  ofs << "\n";
}

void DumpGatherActor(const GatherActor *actor, std::ofstream &ofs) {
  MS_EXCEPTION_IF_NULL(actor);
  ofs << "\tactor_name:" << actor->GetAID().Name() << '\n';
}

void DumpSwitchActor(const SwitchActor *actor, std::ofstream &ofs) {
  MS_EXCEPTION_IF_NULL(actor);
  ofs << "\tactor_name:" << actor->GetAID().Name() << '\n';
}
}  // namespace

void DumpDataPrepareActor(const DataPrepareActorPtr &actor, std::ofstream &ofs) {
  ofs << "\n\n[Data prepare actor:" << (actor != nullptr ? 1 : 0) << "]\n";
  if (actor == nullptr) {
    return;
  }

  ofs << "\tactor_name:" << actor->GetAID().Name() << "\n";
  DumpAbstractActor(actor.get(), ofs);

  ofs << "\t\tcontinuous_memory_nodes:" << actor->continuous_memory_nodes().size() << "\n ";
  for (const auto &iter : actor->continuous_memory_nodes()) {
    MS_EXCEPTION_IF_NULL(iter.first.first);
    MS_EXCEPTION_IF_NULL(iter.first.second);
    ofs << "\t\t\tnode_name:" << iter.first.first->fullname_with_scope()
        << "\tdevice_context:" << iter.first.second->device_context_key().ToString()
        << "\tis_input_need:" << iter.second.first << "\tis_output_need:" << iter.second.second << "\n";
  }
}

void DumpLoopCountActor(const LoopCountActorPtr &actor, std::ofstream &ofs) {
  ofs << "\n\n[Loop count actor:" << (actor != nullptr ? 1 : 0) << "]\n";
  if (actor == nullptr) {
    return;
  }

  ofs << "\tactor_name:" << actor->GetAID().Name() << "\tloop_count:" << actor->loop_count() << "\n";
  DumpAbstractActor(actor.get(), ofs);

  const size_t kOutputControlArrowsNum = 2;
  ofs << "\t\toutput_control_arrows:" << kOutputControlArrowsNum << "\n ";
  ofs << "\t\t\tto_actor_name:" << actor->output_aid().Name() << "\n";
  ofs << "\t\t\tto_actor_name:" << actor->data_prepare_aid().Name() << "\n";
}

void DumpOutputActor(const OutputActorPtr &actor, std::ofstream &ofs) {
  ofs << "\n\n[Output actor:" << (actor != nullptr ? 1 : 0) << "]\n";
  if (actor == nullptr) {
    return;
  }

  ofs << "\tactor_name:" << actor->GetAID().Name() << "\tloop_count:" << actor->loop_count()
      << "\toutputs_num:" << actor->outputs_num() << "\n";

  DumpAbstractActor(actor.get(), ofs);

  ofs << "\t\tinput_result_arrows:" << actor->input_result_arrow_aids().size() << "\n ";
  for (const auto &input_result_arrow_aid : actor->input_result_arrow_aids()) {
    ofs << "\t\t\tfrom_actor_name:" << input_result_arrow_aid.Name() << "\n";
  }

  ofs << "\t\toutput_address_persisted_nodes:" << actor->output_address_persisted_nodes().size() << "\n ";
  for (const auto &output_address_persisted_node : actor->output_address_persisted_nodes()) {
    MS_EXCEPTION_IF_NULL(output_address_persisted_node);
    ofs << "\t\t\toutput_address_persisted_node_name:" << output_address_persisted_node->fullname_with_scope() << "\n";
  }
}

void DumpDSActors(const std::vector<DataSourceActorPtr> &actors, std::ofstream &ofs) {
  ofs << "\n\n[Data source actors:" << actors.size() << "]\n";
  for (const auto &data_source_actor : actors) {
    DumpDSActor(data_source_actor.get(), ofs);
  }
}

void DumpKernelActors(const std::vector<KernelActorPtr> &actors, std::ofstream &ofs) {
  ofs << "\n\n[Kernel actors:" << actors.size() << "]\n";
  for (const auto &kernel_actor : actors) {
    DumpKernelActor(kernel_actor.get(), ofs);
  }
}

void DumpSuperKernelActors(const std::vector<SuperKernelActorPtr> &actors, std::ofstream &ofs) {
  ofs << "\n\n[Super kernel actors:" << actors.size() << "]\n";
  for (const auto &super_kernel_actor : actors) {
    DumpSuperKernelActor(super_kernel_actor.get(), ofs);
  }
}

void DumpNoInputKernelActors(const std::vector<AbstractActorPtr> &actors, std::ofstream &ofs) {
  ofs << "\n\n[No input kernel actors:" << actors.size() << "]\n";
  for (const auto &actor : actors) {
    MS_EXCEPTION_IF_NULL(actor);
    if (actor->type() == KernelTransformType::kKernelActor) {
      auto kernel_actor = dynamic_cast<const KernelActor *>(actor.get());
      MS_EXCEPTION_IF_NULL(kernel_actor);
      DumpKernelActor(kernel_actor, ofs);
    } else if (actor->type() == KernelTransformType::kSuperKernelActor) {
      auto super_kernel_actor = dynamic_cast<const SuperKernelActor *>(actor.get());
      MS_EXCEPTION_IF_NULL(super_kernel_actor);
      DumpSuperKernelActor(super_kernel_actor, ofs);
    }
  }
}

void DumpCopyActors(const std::vector<CopyActorPtr> &actors, std::ofstream &ofs) {
  ofs << "\n\n[Copy actors:" << actors.size() << "]\n";
  for (const auto &copy_actor : actors) {
    DumpCopyActor(copy_actor.get(), ofs);
  }
}

void DumpGatherActors(const std::vector<GatherActorPtr> &actors, std::ofstream &ofs) {
  ofs << "\n\n[Gather actors:" << actors.size() << "]\n";
  for (const auto &gather_actor : actors) {
    DumpGatherActor(gather_actor.get(), ofs);
  }
}

void DumpSwitchActors(const std::vector<SwitchActorPtr> &actors, std::ofstream &ofs) {
  ofs << "\n\n[Switch actors:" << actors.size() << "]\n";
  for (const auto &switch_actor : actors) {
    DumpSwitchActor(switch_actor.get(), ofs);
  }
}
}  // namespace runtime
}  // namespace mindspore