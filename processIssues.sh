#!/usr/bin/env bash
INPUT_PATH="TheReporter_and_TemperanceJournal"
OUTPUT_PATH="/home/jsanford/nbnp/data/output"
ISSUE_COUNTER=1

find $INPUT_PATH -name 'metadata.php' | while read METADATA_FILE; do
  ISSUE_PATH="$OUTPUT_PATH/issue$ISSUE_COUNTER"
  mkdir -p $ISSUE_PATH
  sem -j-2 php -f processIssueFiles.php $METADATA_FILE $ISSUE_PATH
  ISSUE_COUNTER=$((ISSUE_COUNTER+1))
done
sem --wait
