#include "test/unity.h"
#include "test/unity_fixture.h"

#ifdef __GNUC__
// Unity macros trigger this warning
#pragma GCC diagnostic ignored "-Wnested-externs"
#endif

TEST_GROUP_RUNNER(atcacert_def)
{
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_key_id);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_key_id_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_cert_element);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_cert_element_edge);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_cert_element_missing);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_cert_element_unexpected_size);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_cert_element_out_of_bounds);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_cert_element_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_cert_element);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_cert_element_missing);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_cert_element_unexpected_size);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_cert_element_out_of_bounds);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_cert_element_bad_params);
    
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_public_key_add_padding);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_public_key_add_padding_in_place);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_public_key_remove_padding);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_public_key_remove_padding_in_place);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_subj_public_key);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_subj_public_key_bad_params);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_subj_public_key);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_subj_public_key_bad_params);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_subj_key_id);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_subj_key_id_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_signature_non_x509);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_signature_x509_same_size);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_signature_x509_bigger);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_signature_x509_smaller);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_signature_x509_smallest);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_signature_x509_out_of_bounds);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_signature_x509_small_buf);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_signature_x509_bad_cert_length);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_signature_x509_cert_length_change);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_signature_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_signature_non_x509);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_signature_x509_no_padding);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_signature_x509_r_padding);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_signature_x509_rs_padding);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_signature_x509_bad_sig);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_signature_x509_out_of_bounds);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_signature_x509_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_issue_date);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_issue_date_bad_params);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_issue_date);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_issue_date_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_expire_date);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_expire_date_bad_params);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_expire_date);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_expire_date_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_signer_id);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_signer_id_bad_params);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_signer_id_uppercase);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_signer_id_lowercase);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_signer_id_invalid);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_signer_id_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_cert_sn);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_cert_sn_unexpected_size);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_cert_sn_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_stored);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_stored_bad_params);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_device_sn);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_device_sn_unexpected_size);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_device_sn_bad_params);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_signer_id);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_signer_id_unexpected_size);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_signer_id_bad_signer_id);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_signer_id_bad_params);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_pub_key_hash);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_pub_key_hash_pos);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_pub_key_hash_raw);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_pub_key_hash_unexpected_size);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_pub_key_hash_bad_public_key);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_pub_key_hash_bad_issue_date);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_pub_key_hash_bad_params);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_device_sn_hash);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_device_sn_hash_pos);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_device_sn_hash_raw);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_device_sn_hash_unexpected_size);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_device_sn_hash_bad_issue_date);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_gen_cert_sn_device_sn_hash_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_cert_sn);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_cert_sn_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_auth_key_id);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_auth_key_id_bad_params);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_auth_key_id);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_auth_key_id_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_comp_cert_same_size);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_comp_cert_bigger);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_comp_cert_bad_format);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_comp_cert_bad_template_id);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_comp_cert_bad_chain_id);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_comp_cert_bad_sn_source);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_comp_cert_bad_enc_dates);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_set_comp_cert_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_comp_cert);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_comp_cert_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_tbs);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_tbs_bad_cert);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_tbs_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_tbs_digest);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_tbs_digest_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_empty_list);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_devzone_none);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_count0);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_align1);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_align2);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_align3);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_align4);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_align5);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_align6);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_align7);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_align8);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_align9);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_align10);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_align11);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_32block_no_change);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_32block_round_down);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_32block_round_up);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_32block_round_both);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_32block_round_down_merge);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_32block_round_up_merge);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_data_diff_slot);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_data_diff_genkey);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_config);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_otp);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_first);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_mid);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_last);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_add);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_small_buf);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_merge_device_loc_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_device_locs_device);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_device_locs_signer_device);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_device_locs_32block_signer_device);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_device_locs_small_buf);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_device_locs_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_cert_build_start_signer);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_cert_build_process_signer_public_key);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_cert_build_process_signer_comp_cert);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_cert_build_finish_signer);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_cert_build_start_device);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_cert_build_process_device_public_key);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_cert_build_process_device_comp_cert);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_cert_build_finish_device);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_cert_build_start_small_buf);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_cert_build_start_bad_params);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_cert_build_process_bad_params);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_cert_build_finish_missing_device_sn);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_cert_build_finish_bad_params);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_is_device_loc_overlap_align1);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_is_device_loc_overlap_align2);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_is_device_loc_overlap_align3);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_is_device_loc_overlap_align4);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_is_device_loc_overlap_align5);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_is_device_loc_overlap_align6);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_is_device_loc_overlap_align7);
    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_is_device_loc_overlap_align8);

    RUN_TEST_CASE(atcacert_def, atcacert_def__atcacert_get_device_data_flow);
}