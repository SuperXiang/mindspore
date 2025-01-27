/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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

#include <iostream>
#include <fstream>
#include <memory>
#include <algorithm>

#include "profiler/device/profiling.h"
#include "profiler/device/ascend/pynative_profiling.h"
#include "pybind_api/api_register.h"

namespace mindspore {
namespace profiler {
namespace ascend {
std::shared_ptr<PynativeProfiler> PynativeProfiler::profiler_inst_ = std::make_shared<PynativeProfiler>();

std::shared_ptr<PynativeProfiler> &PynativeProfiler::GetInstance() {
  MS_EXCEPTION_IF_NULL(profiler_inst_);
  return profiler_inst_;
}

void PynativeProfiler::Init(const std::string &profileDataPath) {
  MS_LOG(INFO) << "Initialize pynatiave Ascend Profiling";
  profile_data_path_ = profileDataPath;
  enable_flag_ = true;
  std::string device_id = common::GetEnv("RANK_ID");
  if (device_id.empty()) {
    rank_id_ = 0;
  } else {
    rank_id_ = atoi(device_id.c_str());
  }
  WriteStartTime();
}

void PynativeProfiler::WriteStartTime() {
  std::string file_path = profile_data_path_ + "/start_time_" + std::to_string(rank_id_) + ".txt";
  std::ofstream ofs(file_path);
  // check if the file is writable
  if (!ofs.is_open()) {
    MS_LOG(WARNING) << "Open file '" << file_path << "' failed!";
    return;
  }

  // write start time info into file
  try {
    uint64_t device_timestamp = GetRealTimeStamp();
    uint64_t host_monotonic_raw_time = GetHostMonoTimeStamp();
    ofs << "host_monotonic_raw_time(ns): " << host_monotonic_raw_time << std::endl;
    ofs << "device_start_time(ns): " << device_timestamp << std::endl;
  } catch (const std::exception &e) {
    MS_LOG(ERROR) << "Write " << file_path << "failed:" << e.what();
  }
  ofs.close();
  ChangeFileMode(file_path);
  MS_LOG(INFO) << "Write profiler start time infos into file: " << file_path;
}

void PynativeProfiler::SaveProfileData() { WriteOpDetail(profile_data_path_); }

void PynativeProfiler::ClearInst() { pynative_op_info_.clear(); }

void PynativeProfiler::OpDataProducerBegin(AscendKernelRuntime *runtime_instance_, void *stream,
                                           const std::string &op_name) {
  if (enable_flag_ == false) {
    return;
  }

  MS_EXCEPTION_IF_NULL(runtime_instance_);
  start = runtime_instance_->CreateDeviceTimeEvent();
  end = runtime_instance_->CreateDeviceTimeEvent();
  MS_EXCEPTION_IF_NULL(start);
  MS_EXCEPTION_IF_NULL(end);
  start->set_record_stream(stream);
  end->set_record_stream(stream);
  start->RecordEvent();

  op_name_ = op_name;
  stream_ = stream;
}

void PynativeProfiler::StepProfilingEnable(const bool enable_flag) { enable_flag_ = enable_flag; }

void PynativeProfiler::OpDataProducerEnd() {
  if (enable_flag_ == false) {
    return;
  }

  if (start == nullptr || end == nullptr) {
    MS_LOG(WARNING) << "Pynative profiling, the start or end time of op is null"
                    << ", please call the OpDataProducerBegin function first.";
    return;
  }

  float cost_time = 0;
  // Operator asynchronous execution changed to synchronous
  end->RecordEvent();
  start->SyncEvent();
  end->SyncEvent();
  start->ElapsedTime(&cost_time, end.get());

  PynativeOpInfo op_info;
  op_info.op_name = op_name_;
  op_info.stream = stream_;
  op_info.duration = cost_time;

  int64_t milli_second_ratio = 1000;
  int64_t end_timestamp = GetRealTimeStamp();
  int64_t start_timestamp = end_timestamp - static_cast<int64_t>(cost_time * milli_second_ratio);
  double_t start_t = static_cast<double_t>(start_timestamp) / milli_second_ratio;
  op_info.start_timestamp = start_t;

  pynative_op_info_.push_back(op_info);
}

void PynativeProfiler::Stop() {
  SaveProfileData();
  ClearInst();
  enable_flag_ = false;
}

void PynativeProfiler::WriteOpDetail(const std::string &out_path_dir) {
  std::string file_path = out_path_dir + "/output_timeline_data_" + std::to_string(rank_id_) + ".txt";
  std::ofstream ofs(file_path);
  if (!ofs.is_open()) {
    MS_LOG(WARNING) << "Open file '" << file_path << "' failed!";
    return;
  }
  try {
    ofs << "op_name, stream_id, start_time(ms), duration(ms)" << std::endl;
    std::sort(pynative_op_info_.begin(), pynative_op_info_.end(),
              [](const auto &op1, const auto &op2) { return op1.start_timestamp < op2.start_timestamp; });
    for (PynativeOpInfo op_info : pynative_op_info_) {
      ofs << op_info.op_name << ",0," << std::to_string(op_info.start_timestamp) << "," << op_info.duration
          << std::endl;
    }
  } catch (const std::exception &e) {
    MS_LOG(ERROR) << "Write " << file_path << "failed: " << e.what();
  }
  ofs.close();
  ChangeFileMode(file_path);
  MS_LOG(INFO) << "Write " << pynative_op_info_.size() << " op detail infos into file: " << file_path;
}

void PynativeProfiler::ChangeFileMode(const std::string &file_path) const {
  if (chmod(common::SafeCStr(file_path), S_IRUSR | S_IWUSR) == -1) {
    MS_LOG(WARNING) << "Modify file: " << file_path << " to rw fail.";
    return;
  }
}

REGISTER_PYBIND_DEFINE(PynativeProfiler_, ([](const py::module *m) {
                         (void)py::class_<PynativeProfiler, std::shared_ptr<PynativeProfiler>>(*m, "PynativeProfiler")
                           .def_static("get_instance", &PynativeProfiler::GetInstance, "PynativeProfiler get_instance.")
                           .def("init", &PynativeProfiler::Init, py::arg("profile_data_path"), "init")
                           .def("stop", &PynativeProfiler::Stop, "stop");
                       }));
}  // namespace ascend
}  // namespace profiler
}  // namespace mindspore
