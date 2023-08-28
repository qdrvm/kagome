/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string_view>

#include <gtest/gtest.h>
#include <rocksdb/options.h>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include "blockchain/impl/block_header_repository_impl.hpp"  //header_repo
#include "crypto/bip39/impl/bip39_provider_impl.hpp"         //bip39_provider
#include "crypto/crypto_store/crypto_store_impl.hpp"         //crypto_store
#include "crypto/ecdsa/ecdsa_provider_impl.hpp"              //ecdsa_provider
#include "crypto/ed25519/ed25519_provider_impl.hpp"          //ed25519_provider
#include "crypto/hasher/hasher_impl.hpp"                     //hasher
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"       //pbkdf2_provider
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"  //secp256k1_provider
#include "crypto/sr25519/sr25519_provider_impl.hpp"      //sr25519_provider
#include "host_api/impl/host_api_factory_impl.hpp"       // host_api_factory
#include "offchain/impl/offchain_persistent_storage.hpp"  //offchain_persistent_store
#include "offchain/impl/offchain_worker_pool_impl.hpp"  //offchain_worker_pool
#include "runtime/common/runtime_properties_cache_impl.hpp"  // cache
#include "runtime/executor.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module.hpp"  // smc
#include "runtime/runtime_context.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"           // compartment
#include "runtime/wavm/instance_environment_factory.hpp"  // instance_env_factory
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"   // intrinsic_module
#include "runtime/wavm/module_factory_impl.hpp"           // module_factory
#include "runtime/wavm/module_params.hpp"                 //module_params
#include "storage/in_memory/in_memory_storage.hpp"        // storage
#include "storage/rocksdb/rocksdb.hpp"                    //database
#include "storage/trie/impl/trie_storage_backend_impl.hpp"  // storage_backend
#include "storage/trie/impl/trie_storage_impl.hpp"          // trie_storage
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"  // trie_factory
#include "storage/trie/serialization/polkadot_codec.hpp"              //codec
#include "storage/trie/serialization/trie_serializer_impl.hpp"  // serializer
#include "storage/trie_pruner/impl/dummy_pruner.hpp"            // trie_pruner
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/runtime/common/basic_code_provider.hpp"

constexpr std::string_view kBasePath = "./";
constexpr std::string_view kWasmFiles[] = {
    //    "007e2375e1b550f213032134cba005e1acf448797deb897c0cec4dad85ce66ac.wasm",
    //    "009d8183767c977528013b16f77a1bd665f9b922a6287cc44519880a09e49158.wasm",
    //    "03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314.wasm",
    //    "03c90fee7320ef7e675711161fad225831f59f58a2b9563c8ece3c919989f430.wasm",
    //    "09d3331b2a35d695db4c656297ad5af35935c2eabf13b272bb384452abf00945.wasm",
    //    "0a97751c1a7b85d427f002ef5c96cfb351c909432d97cdb53cab1c1e8537ddee.wasm",
    //    "0c1c12b8738e46f93b3cf004799f41fe7db700185ba40ad7397843f4b87e2d6d.wasm",
    //    "0e1223da9a6d2360c0c90f00db84529ae1709588caced86bce09f2d4f2d92e87.wasm",
    //    "0f72fb8e0ef96652558a34f3993e22be0c6f0c50ab267ad597f9165cf3e8909b.wasm",
    //    "1198b2d70ea92049467588299a9325f0bc12fa200e67357d7bef92f2362d83b6.wasm",
    //    "134ec69e097f272ad5e061171037411367b7295a4ce8e3eb9a9c7036d0204274.wasm",
    //    "1517fb58aedba4d3f0196e87094c119c42837e08f321eb543be90963578d84d1.wasm",
    //    "18beaa4006bcf815594db0cad79c2a6e17628a477c2caeef77c85a4604aa4788.wasm",
    //    "1f48f670702c7662e4fe3c0995216555a0198a220201f0e62b3cae9c12538dea.wasm",
    //    "2422f69688bf8d4029034a529ac11c19aa5b9bba84ad1e962bb6c9615a0b64dd.wasm",
    //    "287d2de780f2d6c370d06a4107330e19d68e7d1fdc2ee51f4afe5da00205102e.wasm",
    "31f3006541b9d5558527c76be528a7046373f30b9918f4b7f8c27d4b2c83aae4.wasm",
    "34d2701c6472d4d3f525d7d8b8ed1538d55daf8e273b5b609f63957134036343.wasm",
    "367629f0343db279058d41f00ccc837aa0ed05af16f585e775f00a207670e2a7.wasm",
    "3945c9e3d2de890ff9a3a2c28eb1977c3a5ee0e7d73018d6bc4c142c33268925.wasm",
    "4056714e56a9fdaf9d5f5f21f20fca6a84b2bcc17ef8d5cc5645ad10708415ac.wasm",
    "474d57de0af43510cb5103dfdfa6f7cf60c402776b107d32fbb4ac46019e54ca.wasm",
    "49b27fd63d3448fd221a399094afd79a48c5718ee7aeadd80c44aba1a64de02b.wasm",
    "4dd120baa3497ed64fc72163ade6f55366c3fb0d58c5bcc4e307cba4811e5881.wasm",
    "51a4b65cbac62948604339582cfc103e86a4061ff0e2312582dd17d5961590d4.wasm",
    "52f56c98a681b185c1e855c6746d07346caf8a0cd8c98bd22a1758d1a2ddbbdc.wasm",
    "53a15802ff76d31384add436de7ee4d908df3385e6be7d6dfef19bdee6951d06.wasm",
    "57997d052caf67bfbe1542834881f252eab653d4137122d2d6ec2760c15df311.wasm",
    "57e811fd20fa71840b0cb5994778bcd6b4ce4f436782e4bfd46e31e519258b7b.wasm",
    "5c638820803c3a12b20369b28e10a2f03a51fecee27010461487890e76f351a0.wasm",
    "606d1541c4a0a8b6074caf65e82d179566206754d4cd82cd4c2a082bd069d29a.wasm",
    "6bbea290e276904ac3a6a9192f2ea5428f549d4b212600cd365947d3de021d70.wasm",
    "6d7db67a3b710be82860e8b53184aeab9c6c8e1acb78176888993801eaf9c1d7.wasm",
    "73431ee214068596b37e47fe7211611218e0d4a82c0ec66ff328fa9f646d9850.wasm",
    "748cff124604a12694cbf022d24e2c1fe35ed6a29ac3e2429a910c6f72d3a56d.wasm",
    "74e56957cf4aa0ca0a9482bee127e5ec9bcb8279c05a59a8d1650e7a8f8d0f95.wasm",
    "78ec3e2dd6a8cac6bef72b95e6027729fa5a47b293121099ae3e0b210260a94c.wasm",
    "7b8c139e0ea5189beab1a3c32e5eb32582ab1178c6b38d3713669654dd73df8d.wasm",
    "7bb4edd88b9a72ae47f672e22c137c98d9036fd4194ed9f39021d6e563b90bf7.wasm",
    "7f0bb2bf6ab6b31a38ac9618a9c7e8e07547935346efadcf2b66885352facbf7.wasm",
    "7ff85c3d010117957a46b2bf434b704de9d2d206726f0311e49dce787cbc0e21.wasm",
    "81082d5b02639f67ad43028c86efa3968bd823f03a9a7c462c74c939c9c78ef1.wasm",
    "839321b7f9a5e920bafa2fc38e2709663038a6c0ab51d99e5e361c95e1db0e32.wasm",
    "840f2ad1818414df8b132c77aeffd3fb5a75566389554e854c08190cb7cd960d.wasm",
    "847a359134fe5bc125b1636661a97e4b35aa3b9675fa9f0836ff7e155554254a.wasm",
    "8590eef9cdbde6dee73b1093662245296b3a54cbd6a014a150566ba0ee27202d.wasm",
    "85fc2aacb21039644ec151bc76a8862cd191f340d6884f6765257839911ee53a.wasm",
    "8a68cbaf7ecad8d54edc6d92a15726e8a8ff0885d43d1af67c294291e3891a60.wasm",
    "8bb0894d8ca3a71f484f67fbfcc8c708919290f4e17c2ed84e4933c9b67da48b.wasm",
    "8c52de6c01e6d4a738dbf5ae74d1d00ca461775edbf27f2da9ae71c6a678fd60.wasm",
    "90f407654d4c5e74243a16b7a26c767a2ab08b0188eb693b6b08f86bd33b8946.wasm",
    "92e40ba7d6ad469c4e2d5ad46fd73e533972eb5cafdfb356ec494fdbbb13f2d1.wasm",
    "93db84ea645d9a2d578b6ba264a64084fbb09cac4d1ceea23518c776af879eed.wasm",
    "968a75d0c1e3fb5cd6eeabdbc4d675269c0c236a608e0a08432285bb75820756.wasm",
    "9b3e58d22d5132ce4dd460baac8e0ee4325b649b3cea5e48221d1301ea760a91.wasm",
    "9bb8ab583eaf6a4caa1b0c57c123a697b0dfe7a5935ffff7726b9f359ccc59ad.wasm",
    "a024e192daa80c638f7d82c87d980cc5a8cfcfbc1bebdd1ad0c5319e0953abc9.wasm",
    "a2864ab358a39134d9705fcd1b2d5e5890e62a8bbf0b07ec07a6c3f81fd131a6.wasm",
    "a3d461f53fcbc99625e653b80d5e1f4b0c25fdb0d735e430df79700409320c9a.wasm",
    "a782abdf7c2c84d9916b0b9ecd8c0cd873fe6d1c0dc685b21169660bb05d1dfc.wasm",
    "a8275a0a21d61c898001d7cbea8e426dd6320bbefd61e5f687298ebc4ae365f5.wasm",
    "a96c61cc87e0672bd7e4b10f78eb50dede2d00efb100156327a0a3c0d79052ad.wasm",
    "aa6550e73187a7797001c56609c704d0e8a73cdc02ca9f60646162804b85f05a.wasm",
    "aaf8b111859be1b3d33934ee9bc8a6fdc2bd674f5236099d42bf469cae8e9dde.wasm",
    "ad18f49809977d92440bd2c8c6e33eefa462a1f17bfd81ff1a4fc22745fd8d12.wasm",
    "ae85d245a3d00bfde01f59f3c4fe0b4bfae1cb37e9cf91929eadcea4985711de.wasm",
    "b19f77e93d7dd1cf5818e50f2df34cae0d49f2ba5e025d0bde2b837c285428b6.wasm",
    "ba54b48e2dd20a55969da85fb6923882ff5cbd04cbdc7c66d0c9782dc3b795f5.wasm",
    "bbce7e6a94de42d6e978293f04d9c94fa205a6bfc6e9fcf77ac1f7ec241d69f6.wasm",
    "bf9e11785b4a92b61caf8886d245c88ee8cef85a270f6fb4309839391f56e8c4.wasm",
    "cc67b94d9a88de956305bd1926c0ba869e8e1dbe4efe1eedd5e55a0ce61e20c3.wasm",
    "ccdfc804e0482f951ef7ad15fda0d38ead81c42e93a8276e60a45c663b8a3b91.wasm",
    "df725a1f46de8479543901441ffeec93caad37116297f009b6197d6358c92b5e.wasm",
    "e0ec7babb1fc5a44cc629659a731aeec2569b9c87b137ece431fcc2756d59233.wasm",
    "e2083f7f5dd1d7ec0746e5f4ae444a46e665385ff78c3d2c8e1a675164ae1e2d.wasm",
    "e6d2063f9c50698bbe07b716d99003181c34f69fb44f441cc0c5638334c90f57.wasm",
    "e84672a9c8ccfa9eac82f08b5bec9fedfcf31d978705b608186d71a48f580a2a.wasm",
    "ead2ae37aa6611275f39efdabd292c6338f4ab50f8bea4e8a783b7fe39894e59.wasm",
    "eba8cb2d4511643c91c843ae379a91e7894314c3b63564b31ce50633da3738a2.wasm",
    "ec0aad23795ab56a26ec2524625d747d0d4d036bfb3dd1fd3c87267beb29a01c.wasm",
    "ec57d18d4e49ae63c46eea702d32986fe7049ff643d873dd881933799d49ba25.wasm",
    "ec894750f55574bd094b31bfd8c39034487a200060e593240bf5fc968d672d77.wasm",
    "ef115d61db247aab97924ff316b9abc95d831c8b53313556ab1490e4dfbdfaf2.wasm",
    "f7a564ae14804ecf6be5c224b81549fcd1c8990a333bd3f58b58f79376f6dcaf.wasm",
};

class WavmModuleInitTest : public ::testing::TestWithParam<std::string_view> {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    auto compartment =
        std::make_shared<kagome::runtime::wavm::CompartmentWrapper>(
            "WAVM Compartment");
    auto module_params =
        std::make_shared<kagome::runtime::wavm::ModuleParams>();

    auto trie_factory =
        std::make_shared<kagome::storage::trie::PolkadotTrieFactoryImpl>();
    auto codec = std::make_shared<kagome::storage::trie::PolkadotCodec>();
    auto storage = std::make_shared<kagome::storage::InMemoryStorage>();
    auto storage_backend =
        std::make_shared<kagome::storage::trie::TrieStorageBackendImpl>(
            storage);
    auto serializer =
        std::make_shared<kagome::storage::trie::TrieSerializerImpl>(
            trie_factory, codec, storage_backend);
    auto state_pruner =
        std::make_shared<kagome::storage::trie_pruner::DummyPruner>();
    std::shared_ptr<kagome::storage::trie::TrieStorageImpl> trie_storage =
        kagome::storage::trie::TrieStorageImpl::createEmpty(
            trie_factory, codec, serializer, state_pruner)
            .value();
    auto intrinsic_module =
        std::make_shared<kagome::runtime::wavm::IntrinsicModule>(
            compartment, module_params->intrinsicMemoryType);

    auto hasher = std::make_shared<kagome::crypto::HasherImpl>();
    auto sr25519_provider =
        std::make_shared<kagome::crypto::Sr25519ProviderImpl>();
    auto ecdsa_provider =
        std::make_shared<kagome::crypto::EcdsaProviderImpl>(hasher);
    auto ed25519_provider =
        std::make_shared<kagome::crypto::Ed25519ProviderImpl>(hasher);
    auto secp256k1_provider =
        std::make_shared<kagome::crypto::Secp256k1ProviderImpl>();
    auto pbkdf2_provider =
        std::make_shared<kagome::crypto::Pbkdf2ProviderImpl>();
    auto bip39_provider = std::make_shared<kagome::crypto::Bip39ProviderImpl>(
        pbkdf2_provider, hasher);
    auto ecdsa_suite =
        std::make_shared<kagome::crypto::EcdsaSuite>(ecdsa_provider);
    auto ed_suite =
        std::make_shared<kagome::crypto::Ed25519Suite>(ed25519_provider);
    auto sr_suite =
        std::make_shared<kagome::crypto::Sr25519Suite>(sr25519_provider);
    std::shared_ptr<kagome::crypto::KeyFileStorage> key_fs =
        kagome::crypto::KeyFileStorage::createAt(
            "/tmp/kagome_vawm_tmp_key_storage")
            .value();
    auto csprng =
        std::make_shared<libp2p::crypto::random::BoostRandomGenerator>();
    auto crypto_store = std::make_shared<kagome::crypto::CryptoStoreImpl>(
        ecdsa_suite, ed_suite, sr_suite, bip39_provider, csprng, key_fs);

    rocksdb::Options db_options{};
    db_options.create_if_missing = true;
    std::shared_ptr<kagome::storage::RocksDb> database =
        kagome::storage::RocksDb::create("/tmp/kagome_tmp_db", db_options)
            .value();

    auto header_repo =
        std::make_shared<kagome::blockchain::BlockHeaderRepositoryImpl>(
            database, hasher);
    auto offchain_persistent_storage =
        std::make_shared<kagome::offchain::OffchainPersistentStorageImpl>(
            database);
    auto offchain_worker_pool =
        std::make_shared<kagome::offchain::OffchainWorkerPoolImpl>();

    auto host_api_factory =
        std::make_shared<kagome::host_api::HostApiFactoryImpl>(
            kagome::host_api::OffchainExtensionConfig{},
            sr25519_provider,
            ecdsa_provider,
            ed25519_provider,
            secp256k1_provider,
            hasher,
            crypto_store,
            offchain_persistent_storage,
            offchain_worker_pool);

    auto smc = std::make_shared<kagome::runtime::SingleModuleCache>();
    auto cache =
        std::make_shared<kagome::runtime::RuntimePropertiesCacheImpl>();

    auto instance_env_factory = std::make_shared<
        const kagome::runtime::wavm::InstanceEnvironmentFactory>(
        trie_storage,
        serializer,
        compartment,
        module_params,
        intrinsic_module,
        host_api_factory,
        header_repo,
        smc,
        cache);

    module_factory_ =
        std::make_shared<kagome::runtime::wavm::ModuleFactoryImpl>(
            compartment,
            module_params,
            instance_env_factory,
            intrinsic_module,
            std::nullopt,
            hasher);
  }

  std::shared_ptr<kagome::runtime::ModuleFactory> module_factory_;
  kagome::log::Logger log_ = kagome::log::createLogger("Test");
};

TEST_P(WavmModuleInitTest, SingleModule) {
  auto wasm = GetParam();
  SL_INFO(log_, "Trying {}", wasm);
  auto code_provider = std::make_shared<kagome::runtime::BasicCodeProvider>(
      std::string(kBasePath) + std::string(wasm));
  EXPECT_OUTCOME_TRUE(code, code_provider->getCodeAt({}));
  EXPECT_OUTCOME_TRUE(
      runtime_context,
      kagome::runtime::RuntimeContextFactory::fromCode(*module_factory_, code));
  EXPECT_OUTCOME_TRUE(
      memory_response,
      runtime_context.module_instance->callExportFunction("Core_version", {}));
  auto memory = runtime_context.module_instance->getEnvironment()
                    .memory_provider->getCurrentMemory();
  GTEST_ASSERT_TRUE(memory.has_value());
  auto scale_resp =
      memory->get().loadN(memory_response.ptr, memory_response.size);
  EXPECT_OUTCOME_TRUE(cv,
                      scale::decode<kagome::primitives::Version>(scale_resp));
  SL_INFO(log_,
          "Module initialized - spec {}-{}, impl {}-{}",
          cv.spec_name,
          cv.spec_version,
          cv.impl_name,
          cv.impl_version);
}

INSTANTIATE_TEST_SUITE_P(SingleModule,
                         WavmModuleInitTest,
                         testing::ValuesIn(kWasmFiles));
