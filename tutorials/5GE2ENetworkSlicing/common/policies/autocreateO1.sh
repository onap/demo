#!/bin/sh
echo "Generating and pushing policies..."
python3 policy_utils.py create_policy_types policy_types
python3 policy_utils.py create_and_push_policies nst_policies
python3 policy_utils.py generate_nsi_policies NSTO1
python3 policy_utils.py create_and_push_policies gen_nsi_policies
cp -TR trtnsst nsi_policies
python3 policy_utils.py generate_nsi_policies TESTRANTOPNSST
python3 policy_utils.py create_and_push_policies gen_nsi_policies
cp -TR bak_nsi_policies nsi_policies
python3 policy_utils.py generate_nssi_policies RAN_NF_NSST minimize latency
python3 policy_utils.py create_and_push_policies gen_nssi_policies
python3 policy_utils.py generate_nssi_policies TN_BH_NSST minimize latency
python3 policy_utils.py create_and_push_policies gen_nssi_policies
python3 policy_utils.py generate_nssi_policies CN_NSST minimize latency
python3 policy_utils.py create_and_push_policies gen_nssi_policies
echo "Done."
