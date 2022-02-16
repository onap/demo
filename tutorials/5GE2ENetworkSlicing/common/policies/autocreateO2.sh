#!/bin/sh
echo "Generating and pushing policies..."
python3 policy_utils.py create_policy_types policy_types
python3 policy_utils.py create_and_push_policies nst_policies
python3 policy_utils.py generate_nsi_policies EmbbNst_O2
python3 policy_utils.py create_and_push_policies gen_nsi_policies
python3 policy_utils.py generate_nssi_policies EmbbAn_NF minimize latency
python3 policy_utils.py create_and_push_policies gen_nssi_policies
python3 policy_utils.py generate_nssi_policies TN_ONAP_internal_BH minimize latency
python3 policy_utils.py create_and_push_policies gen_nssi_policies
python3 policy_utils.py generate_nssi_policies EmbbCn_External minimize latency
python3 policy_utils.py create_and_push_policies gen_nssi_policies
echo "Done."
