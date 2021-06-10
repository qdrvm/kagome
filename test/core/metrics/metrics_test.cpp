#include "metrics/metrics.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <thread>
#include "metrics/impl/prometheus/registry_impl.hpp"

template <typename T>
prometheus::ClientMetric getMetric(T *metric) {
  return kagome::metrics::PrometheusRegistry::internalMetric(metric)->Collect();
}

class CounterTest : public ::testing::Test {
  kagome::metrics::RegistryPtr registry_;

 public:
  void SetUp() override {
    registry_ = kagome::metrics::createRegistry();
  }

  kagome::metrics::Counter *createCounter(const std::string &name) {
    registry_->registerCounterFamily(name);
    return registry_->registerCounterMetric(name);
  }
};

// all tests have same static prometheus::Registry

/**
 * @given an empty registry
 * @when putting an empty counter
 * @then expected result obtained
 */
TEST_F(CounterTest, InitializeWithZero) {
  auto counter = createCounter("counter1");
  EXPECT_DOUBLE_EQ(getMetric(counter).counter.value, 0.0);
}

/**
 * @given prev registry
 * @when putting a counter and incrementing
 * @then expected result obtained
 */
TEST_F(CounterTest, Inc) {
  auto counter = createCounter("counter2");
  counter->inc();
  EXPECT_DOUBLE_EQ(getMetric(counter).counter.value, 1.0);
}

/**
 * @given prev registry
 * @when putting a counter and incrementing by value
 * @then expected result obtained
 */
TEST_F(CounterTest, IncNumber) {
  auto counter = createCounter("counter3");
  counter->inc(4.0);
  EXPECT_DOUBLE_EQ(getMetric(counter).counter.value, 4.0);
}

/**
 * @given prev registry
 * @when putting a counter and incrementing sequentially
 * @then expected result obtained
 */
TEST_F(CounterTest, IncMultiple) {
  auto counter = createCounter("counter4");
  counter->inc();
  counter->inc();
  counter->inc(5.0);
  EXPECT_DOUBLE_EQ(getMetric(counter).counter.value, 7.0);
}

/**
 * @given prev registry
 * @when putting a counter and incrementing by negative value
 * @then expected result obtained
 */
TEST_F(CounterTest, IncNegativeValue) {
  auto counter = createCounter("counter5");
  counter->inc(5.0);
  counter->inc(-5.0);
  EXPECT_DOUBLE_EQ(getMetric(counter).counter.value, 5.0);
}

class GaugeTest : public ::testing::Test {
  kagome::metrics::RegistryPtr registry_;

 public:
  void SetUp() override {
    registry_ = kagome::metrics::createRegistry();
  }

  kagome::metrics::Gauge *createGauge(const std::string &name) {
    registry_->registerGaugeFamily(name);
    return registry_->registerGaugeMetric(name);
  }
};

/**
 * @given prev registry
 * @when putting an empty gauge
 * @then expected result obtained
 */
TEST_F(GaugeTest, InitializeWithZero) {
  auto gauge = createGauge("gauge1");
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 0);
}

/**
 * @given prev registry
 * @when putting a gauge and incrementing
 * @then expected result obtained
 */
TEST_F(GaugeTest, Inc) {
  auto gauge = createGauge("gauge2");
  gauge->inc();
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 1.0);
}

/**
 * @given prev registry
 * @when putting a gauge and incrementing by value
 * @then expected result obtained
 */
TEST_F(GaugeTest, IncNumber) {
  auto gauge = createGauge("gauge3");
  gauge->inc(4);
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 4.0);
}

/**
 * @given prev registry
 * @when putting a gauge and incrementing sequentially
 * @then expected result obtained
 */
TEST_F(GaugeTest, IncMultiple) {
  auto gauge = createGauge("gauge4");
  gauge->inc();
  gauge->inc();
  gauge->inc(5);
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 7.0);
}

/**
 * @given prev registry
 * @when putting a gauge and incrementing by negative value
 * @then expected result obtained
 */
TEST_F(GaugeTest, IncNegativeValue) {
  auto gauge = createGauge("gauge5");
  gauge->inc(-1.0);
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, -1.0);
}

/**
 * @given prev registry
 * @when putting a gauge and applying few operations
 * @then expected result obtained
 */
TEST_F(GaugeTest, Dec) {
  auto gauge = createGauge("gauge6");
  gauge->set(5.0);
  gauge->dec();
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 4.0);
}

/**
 * @given prev registry
 * @when putting a gauge and decreasing by negative value
 * @then expected result obtained
 */
TEST_F(GaugeTest, DecNegativeValue) {
  auto gauge = createGauge("gauge7");
  gauge->dec(-1.0);
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 1.0);
}

/**
 * @given prev registry
 * @when putting a gauge and applying few operations
 * @then expected result obtained
 */
TEST_F(GaugeTest, DecNumber) {
  auto gauge = createGauge("gauge8");
  gauge->set(5.0);
  gauge->dec(3.0);
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 2.0);
}

/**
 * @given prev registry
 * @when putting a gauge and setting a value
 * @then expected result obtained
 */
TEST_F(GaugeTest, Set) {
  auto gauge = createGauge("gauge9");
  gauge->set(3.0);
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 3.0);
}

/**
 * @given prev registry
 * @when putting a gauge and setting few values sequentially
 * @then expected result obtained
 */
TEST_F(GaugeTest, SetMultiple) {
  auto gauge = createGauge("gauge10");
  gauge->set(3.0);
  gauge->set(8.0);
  gauge->set(1.0);
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 1.0);
}

class HistogramTest : public ::testing::Test {
  kagome::metrics::RegistryPtr registry_;

 public:
  void SetUp() override {
    registry_ = kagome::metrics::createRegistry();
  }

  kagome::metrics::Histogram *createHistogram(
      const std::string &name, const std::vector<double> &bucket_boundaries) {
    registry_->registerHistogramFamily(name);
    return registry_->registerHistogramMetric(name, bucket_boundaries);
  }
};

/**
 * @given prev registry
 * @when putting an empty histogram
 * @then expected result obtained
 */
TEST_F(HistogramTest, InitializeWithZero) {
  auto histogram = createHistogram("histogram1", {});
  auto h = getMetric(histogram).histogram;
  EXPECT_EQ(h.sample_count, 0U);
  EXPECT_DOUBLE_EQ(h.sample_sum, 0);
}

/**
 * @given prev registry
 * @when putting a histogram and obsering few values
 * @then expected result obtained
 */
TEST_F(HistogramTest, SampleCount) {
  auto histogram = createHistogram("histogram2", {1});
  histogram->observe(0);
  histogram->observe(200);
  auto h = getMetric(histogram).histogram;
  EXPECT_EQ(h.sample_count, 2U);
}

/**
 * @given prev registry
 * @when putting a histogram and obsering few values
 * @then expected result obtained
 */
TEST_F(HistogramTest, SampleSum) {
  auto histogram = createHistogram("histogram3", {1});
  histogram->observe(0);
  histogram->observe(1);
  histogram->observe(101);
  auto h = getMetric(histogram).histogram;
  EXPECT_DOUBLE_EQ(h.sample_sum, 102);
}

/**
 * @given prev registry
 * @when putting a histogram and changing backets
 * @then expected result obtained
 */
TEST_F(HistogramTest, BucketSize) {
  auto histogram = createHistogram("histogram4", {1, 2});
  auto h = getMetric(histogram).histogram;
  EXPECT_EQ(h.bucket.size(), 3U);
}

/**
 * @given prev registry
 * @when putting a histogram and changing backets
 * @then expected correct bucket bounds
 */
TEST_F(HistogramTest, BucketBounds) {
  auto histogram = createHistogram("histogram5", {1, 2});
  auto h = getMetric(histogram).histogram;
  EXPECT_DOUBLE_EQ(h.bucket.at(0).upper_bound, 1);
  EXPECT_DOUBLE_EQ(h.bucket.at(1).upper_bound, 2);
  EXPECT_DOUBLE_EQ(h.bucket.at(2).upper_bound,
                   std::numeric_limits<double>::infinity());
}

/**
 * @given prev registry
 * @when putting a histogram and changing backets
 * @then expected correct cumulative count
 */
TEST_F(HistogramTest, BucketCountsNotResetByCollection) {
  auto histogram = createHistogram("histogram6", {1, 2});
  histogram->observe(1.5);
  getMetric(histogram);
  histogram->observe(1.5);
  auto h = getMetric(histogram).histogram;
  ASSERT_EQ(h.bucket.size(), 3U);
  EXPECT_EQ(h.bucket.at(1).cumulative_count, 2U);
}

/**
 * @given prev registry
 * @when putting a histogram and obsering few values
 * @then expected result obtained
 */
TEST_F(HistogramTest, CumulativeBucketCount) {
  auto histogram = createHistogram("histogram7", {1, 2});
  histogram->observe(0);
  histogram->observe(0.5);
  histogram->observe(1);
  histogram->observe(1.5);
  histogram->observe(1.5);
  histogram->observe(2);
  histogram->observe(3);
  auto h = getMetric(histogram).histogram;
  ASSERT_EQ(h.bucket.size(), 3U);
  EXPECT_EQ(h.bucket.at(0).cumulative_count, 3U);
  EXPECT_EQ(h.bucket.at(1).cumulative_count, 6U);
  EXPECT_EQ(h.bucket.at(2).cumulative_count, 7U);
}

/**
 * @given prev registry
 * @when putting a histogram and observing negative value
 * @then expected result obtained
 */
TEST_F(HistogramTest, SumCanGoDown) {
  auto histogram = createHistogram("histogram8", {1});
  auto histogram1 = getMetric(histogram).histogram;
  histogram->observe(-10);
  auto histogram2 = getMetric(histogram).histogram;
  EXPECT_LT(histogram2.sample_sum, histogram1.sample_sum);
}

class SummaryTest : public ::testing::Test {
  kagome::metrics::RegistryPtr registry_;

 public:
  void SetUp() override {
    registry_ = kagome::metrics::createRegistry();
  }

  kagome::metrics::Summary *createSummary(
      const std::string &name,
      const std::vector<std::pair<double, double>> &quantiles,
      std::chrono::milliseconds max_age = std::chrono::seconds{60},
      int age_buckets = 5) {
    registry_->registerSummaryFamily(name);
    return registry_->registerSummaryMetric(
        name, quantiles, max_age, age_buckets);
  }
};

/**
 * @given prev registry
 * @when putting an empty summary
 * @then expected result obtained
 */
TEST_F(SummaryTest, InitializeWithZero) {
  auto summary = createSummary("summary1", {});
  auto s = getMetric(summary).summary;
  EXPECT_EQ(s.sample_count, 0U);
  EXPECT_DOUBLE_EQ(s.sample_sum, 0);
}

/**
 * @given prev registry
 * @when putting a summary and observing few variables
 * @then expected result obtained
 */
TEST_F(SummaryTest, SampleCount) {
  auto summary = createSummary("summary2", {{0.5, 0.05}});
  summary->observe(0);
  summary->observe(200);
  auto s = getMetric(summary).summary;
  EXPECT_EQ(s.sample_count, 2U);
}

/**
 * @given prev registry
 * @when putting a summary and observing few variables
 * @then expected result obtained
 */
TEST_F(SummaryTest, SampleSum) {
  auto summary = createSummary("summary3", {{0.5, 0.05}});
  summary->observe(0);
  summary->observe(1);
  summary->observe(101);
  auto s = getMetric(summary).summary;
  EXPECT_DOUBLE_EQ(s.sample_sum, 102);
}

/**
 * @given prev registry
 * @when putting a summary and applying 2 quantiles
 * @then expected quantile size
 */
TEST_F(SummaryTest, QuantileSize) {
  auto summary = createSummary("summary4", {{0.5, 0.05}, {0.90, 0.01}});
  auto s = getMetric(summary).summary;
  EXPECT_EQ(s.quantile.size(), 2U);
}

/**
 * @given prev registry
 * @when putting a summary and applying 3 quantiles
 * @then expected correct quantile bounds
 */
TEST_F(SummaryTest, QuantileBounds) {
  auto summary =
      createSummary("summary5", {{0.5, 0.05}, {0.90, 0.01}, {0.99, 0.001}});
  auto s = getMetric(summary).summary;
  ASSERT_EQ(s.quantile.size(), 3U);
  EXPECT_DOUBLE_EQ(s.quantile.at(0).quantile, 0.5);
  EXPECT_DOUBLE_EQ(s.quantile.at(1).quantile, 0.9);
  EXPECT_DOUBLE_EQ(s.quantile.at(2).quantile, 0.99);
}

/**
 * @given prev registry
 * @when putting a summary and applying 3 quantiles
 * @then expected correct quantile values
 */
TEST_F(SummaryTest, QuantileValues) {
  static const int SAMPLES = 10000;

  auto summary =
      createSummary("summary6", {{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}});
  for (int i = 1; i <= SAMPLES; ++i) summary->observe(i);

  auto s = getMetric(summary).summary;
  ASSERT_EQ(s.quantile.size(), 3U);

  EXPECT_NEAR(s.quantile.at(0).value, 0.5 * SAMPLES, 0.05 * SAMPLES);
  EXPECT_NEAR(s.quantile.at(1).value, 0.9 * SAMPLES, 0.01 * SAMPLES);
  EXPECT_NEAR(s.quantile.at(2).value, 0.99 * SAMPLES, 0.001 * SAMPLES);
}

/**
 * @given prev registry
 * @when putting a summary and sleeping few times
 * @then expected correct summary values in time
 * @note problematic test if continues to fail on macos consider removing/rewriting
 */
TEST_F(SummaryTest, MaxAge) {
  auto summary =
      createSummary("summary7", {{0.99, 0.001}}, std::chrono::milliseconds(80), 2);
  summary->observe(8.0);

  static const auto test_value = [&summary](double ref) {
    auto s = getMetric(summary).summary;
    ASSERT_EQ(s.quantile.size(), 1U);

    if (std::isnan(ref))
      EXPECT_TRUE(std::isnan(s.quantile.at(0).value));
    else
      EXPECT_DOUBLE_EQ(s.quantile.at(0).value, ref);
  };

  test_value(8.0);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  test_value(8.0);
  std::this_thread::sleep_for(std::chrono::milliseconds(110));
  test_value(std::numeric_limits<double>::quiet_NaN());
}
