//
// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef ZETASQL_LOCAL_SERVICE_LOCAL_SERVICE_H_
#define ZETASQL_LOCAL_SERVICE_LOCAL_SERVICE_H_

#include <stddef.h>

#include <cstdint>
#include <memory>

#include "zetasql/local_service/local_service.pb.h"
#include "zetasql/proto/options.pb.h"
#include "zetasql/public/analyzer.h"
#include "zetasql/public/parse_resume_location.h"
#include "zetasql/public/parse_resume_location.pb.h"
#include <cstdint>
#include "absl/strings/string_view.h"
#include "zetasql/base/status.h"

namespace zetasql {
namespace local_service {

class PreparedExpressionPool;
class PreparedExpressionState;
class RegisteredDescriptorPoolState;
class RegisteredDescriptorPoolPool;
class RegisteredCatalogPool;
class RegisteredCatalogState;

// Implementation of ZetaSqlLocalService RPC service.
class ZetaSqlLocalServiceImpl {
 public:
  ZetaSqlLocalServiceImpl();
  ZetaSqlLocalServiceImpl(const ZetaSqlLocalServiceImpl&) = delete;
  ZetaSqlLocalServiceImpl& operator=(const ZetaSqlLocalServiceImpl&) =
      delete;
  ~ZetaSqlLocalServiceImpl();

  absl::Status Prepare(const PrepareRequest& request,
                       PrepareResponse* response);

  absl::Status Unprepare(int64_t id);

  absl::Status Evaluate(const EvaluateRequest& request,
                        EvaluateResponse* response);

  absl::Status EvaluateImpl(const EvaluateRequest& request,
                            PreparedExpressionState* state,
                            EvaluateResponse* response);

  absl::Status GetTableFromProto(const TableFromProtoRequest& request,
                                 SimpleTableProto* response);

  absl::Status GetBuiltinFunctions(
      const ZetaSQLBuiltinFunctionOptionsProto& proto,
      GetBuiltinFunctionsResponse* resp);

  absl::Status Analyze(const AnalyzeRequest& request,
                       AnalyzeResponse* response);

  absl::Status AnalyzeImpl(
      const AnalyzeRequest& request,
      const std::vector<const google::protobuf::DescriptorPool*>& pools,
      // Analyzer requires a non-const catalog, however
      // is not mutated in practice.
      Catalog* catalog, AnalyzeResponse* response);

  absl::Status AnalyzeExpressionImpl(
      const AnalyzeRequest& request,
      const std::vector<const google::protobuf::DescriptorPool*>& pools,
      // Analyzer requires a non-const catalog, however
      // is not mutated in practice.
      Catalog* catalog, AnalyzeResponse* response);

  absl::Status BuildSql(const BuildSqlRequest& request,
                        BuildSqlResponse* response);

  // Note, this also can handle scripts.
  absl::Status ExtractTableNamesFromStatement(
      const ExtractTableNamesFromStatementRequest& request,
      ExtractTableNamesFromStatementResponse* response);

  // Note, this does not handle scripts.
  absl::Status ExtractTableNamesFromNextStatement(
      const ExtractTableNamesFromNextStatementRequest& request,
      ExtractTableNamesFromNextStatementResponse* response);

  absl::Status SerializeResolvedOutput(
      const AnalyzerOutput* output,
      const std::vector<const google::protobuf::DescriptorPool*>& pools,
      absl::string_view statement, AnalyzeResponse* response);

  absl::Status FormatSql(const FormatSqlRequest& request,
                         FormatSqlResponse* response);

  absl::Status RegisterCatalog(const RegisterCatalogRequest& request,
                               RegisterResponse* response);

  absl::Status UnregisterCatalog(int64_t id);

  absl::Status GetLanguageOptions(const LanguageOptionsRequest& request,
                                  LanguageOptionsProto* response);

  absl::Status GetAnalyzerOptions(const AnalyzerOptionsRequest& request,
                                  AnalyzerOptionsProto* response);

 private:
  absl::Status GetDescriptorPools(
      const DescriptorPoolListProto& descriptor_pool_list,
      std::vector<std::shared_ptr<RegisteredDescriptorPoolState>>&
          pool_states_out);

  template <typename RequestProto>
  absl::Status GetCatalogState(
      const RequestProto& request,
      const std::vector<const google::protobuf::DescriptorPool*>& pools,
      std::shared_ptr<RegisteredCatalogState>& state);

  std::unique_ptr<RegisteredDescriptorPoolPool> registered_descriptor_pools_;
  std::unique_ptr<RegisteredCatalogPool> registered_catalogs_;
  std::unique_ptr<PreparedExpressionPool> prepared_expressions_;

  absl::Status RegisterPrepared(std::unique_ptr<PreparedExpressionState>& state,
                                PreparedState* response);

  // For testing.
  size_t NumSavedPreparedExpression() const;

  friend class ZetaSqlLocalServiceImplTest;
};

}  // namespace local_service
}  // namespace zetasql

#endif  // ZETASQL_LOCAL_SERVICE_LOCAL_SERVICE_H_
