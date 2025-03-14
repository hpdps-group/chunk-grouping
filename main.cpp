#include "3party/cpptoml.h"
#include "config.h"
#include "delta_compression.h"
#include "pipeline_delta_compression.h"
#include <filesystem>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <memory>
#include <string>
DEFINE_string(config, "odess.toml", "path to config file");
using namespace Delta;
int main(int argc, char *argv[]) {
  /* Setup logging. */
  FLAGS_stderrthreshold = google::INFO;
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  FLAGS_log_dir = "./logs";
  FLAGS_logtostderr = false;
  LOG(INFO) << "using config file " << FLAGS_config;

  /* Load config. */
  Config::Instance().Init(FLAGS_config);
  auto task = Config::Instance().get()->get_as<std::string>("task"); // 获取task
  if (*task == "compression") {
    Delta::DeltaCompression *compression;
    auto pipeline = Config::Instance()
                        .get()
                        ->get_as<bool>("pipeline")
                        .value_or<bool>(false);
    if (pipeline) { // 流水线模式
      compression = new Delta::PipelineDeltaCompression();
    } else { // 正常模式
      compression = new Delta::DeltaCompression();
    }
    auto task_data_dir =
        Config::Instance().get()->get_as<std::string>("task_data_dir"); // 数据集文件
    for (const auto &entry :
         std::filesystem::recursive_directory_iterator(*task_data_dir)) { // 遍历数据集
      if (entry.is_regular_file()) {  // 常规文件（非目录或者符号链接等文件）
        LOG(INFO) << "start processing file "
                  << entry.path().relative_path().string(); // 输出文件相对路径
        compression->AddFile(entry.path().relative_path().string()); // 添加到压缩队列
      }
    }
    delete compression;
  }
  return 0;
}