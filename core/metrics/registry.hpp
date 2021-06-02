#ifndef KAGOME_CORE_METRICS_REGISTRY_HPP
#define KAGOME_CORE_METRICS_REGISTRY_HPP

#include <chrono>
#include <map>
#include <string>
#include <vector>

namespace kagome::metrics {

  class Counter;
  class Gauge;
  class Handler;
  class Histogram;
  class Summary;

  /**
   * @brief the class stores metrics, provides interface to create metrics and
   * families of metrics
   * TODO(sanblch) rethink interface to avoid error on calling registering
   * metrics before registering family
   * provides interfaces to register families and metrics of metric types:
   * counter, gauge, histogram, summary
   * @param name Set the metric name.
   * @param help Set an additional description.
   * @param labels Assign a set of key-value pairs (= labels) to the
   * metric. All these labels are propagated to each time series within the
   * metric.
   */
  class Registry {
   public:
    virtual ~Registry() = default;
    virtual void setHandler(Handler &handler) = 0;

    virtual void registerCounterFamily(
        const std::string &name,
        const std::string &help = "",
        const std::map<std::string, std::string> &labels = {}) = 0;

    virtual void registerGaugeFamily(
        const std::string &name,
        const std::string &help = "",
        const std::map<std::string, std::string> &labels = {}) = 0;

    virtual void registerHistogramFamily(
        const std::string &name,
        const std::string &help = "",
        const std::map<std::string, std::string> &labels = {}) = 0;

    virtual void registerSummaryFamily(
        const std::string &name,
        const std::string &help = "",
        const std::map<std::string, std::string> &labels = {}) = 0;

    /**
     * @brief create counter metrics object
     * @param name the name given at call `registerCounterFamily`
     * @return pointer without ownership
     * @note avoid calling before registerCounterFamily
     */
    virtual Counter *registerCounterMetric(
        const std::string &name,
        const std::map<std::string, std::string> &labels = {}) = 0;

    /**
     * @brief create gauge metrics object
     * @param name the name given at call `registerGaugeFamily`
     * @return pointer without ownership
     * @note avoid calling before registerGaugeFamily
     */
    virtual Gauge *registerGaugeMetric(
        const std::string &name,
        const std::map<std::string, std::string> &labels = {}) = 0;

    /**
     * @brief create histogram metrics object
     * @param name the name given at call `registerHistogramFamily`
     * @param bucket_boundaries a list of monotonically increasing values
     * @return pointer without ownership
     * See https://prometheus.io/docs/practices/histograms/ for detailed
     * explanations of histogram usage and differences to summaries.
     * @note avoid calling before registerHistogramFamily
     */
    virtual Histogram *registerHistogramMetric(
        const std::string &name,
        const std::vector<double> &bucket_boundaries,
        const std::map<std::string, std::string> &labels = {}) = 0;

    /**
     * @brief create summary metrics object
     * @param name the name given at call `registerSummaryFamily`
     * @param quantiles a list of pairs denoting an error
     * @param max_age duration of time window
     * @param age_buckets number of buckets of the time window
     * @return pointer without ownership
     * See https://prometheus.io/docs/practices/histograms/ for detailed
     * explanations of Phi-quantiles, summary usage, and differences to
     * histograms.
     * @note avoid calling before registerSummaryFamily
     */
    virtual Summary *registerSummaryMetric(
        const std::string &name,
        const std::vector<std::pair<double, double>> &quantiles,
        std::chrono::milliseconds max_age = std::chrono::seconds{60},
        int age_buckets = 5,
        const std::map<std::string, std::string> &labels = {}) = 0;
  };

}  // namespace kagome::metrics

#endif  // KAGOME_CORE_METRICS_REGISTRY_HPP
