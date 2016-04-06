<?php
/**
 * @file
 * Process newspaper issue pages into the islandora newspaper batch format.
 */

include_once 'NBNDProcessor/NBNDProcessor.inc';
$metadata_file_path = $argv[1];
$target_path = $argv[2];

$issue_processor = new NBNDProcessor($metadata_file_path, $target_path);
$issue_processor->process();
