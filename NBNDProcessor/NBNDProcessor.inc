<?php
/**
 * @file
 * Process newspaper issue pages into the islandora newspaper batch format.
 */

define('NBNDPROCESSOR_HOCR_SURROGATE_ARGS', '-colorspace Gray -threshold 50% -depth 1 +compress');
define('NBNDPROCESSOR_ISLANDORA_JPG_SIZE', '600x800');
define('NBNDPROCESSOR_ISLANDORA_TN_SIZE', '150x200');
define('NBNDPROCESSOR_ISSUE_NUMBER_LENGTH', 3);
define('NBNDPROCESSOR_KAKADU_ARGS', '-rate 0.5 Clayers=1 Clevels=7 "Cprecincts={256,256},{256,256},{256,256},{128,128},{128,128},{64,64},{64,64},{32,32},{16,16}" "Corder=RPCL" "ORGgen_plt=yes" "ORGtparts=R" "Cblk={32,32}" Cuse_sop=yes');
define('NBNDPROCESSOR_TESSERACT_LANGUAGE', 'eng');
define('NBNDPROCESSOR_TESSERACT_TIMEOUT', 600);

/**
 * A generic object to process files from a newspaper issue.
 */
class NBNDProcessor {

  /**
   * The current page directory when iterating inside the issue.
   *
   * @var string
   */
  public $curPageDir = NULL;

  /**
   * The current working file.
   *
   * @var string
   */
  public $curWorkFile = NULL;

  /**
   * The TIF formatted surrogate for OCR.
   *
   * @var string
   */
  public $hocrSurrogate = NULL;

  /**
   * The path to the metadata.php file.
   *
   * @var string
   */
  public $metadataPath = NULL;

  /**
   * The pages in the issue.
   *
   * @var array
   */
  public $pages = array();

  /**
   * The directory containing the source page files.
   *
   * @var string
   */
  public $pagesDirectory = NULL;

  /**
   * The target path to write the issue output.
   *
   * @var string
   */
  public $targetPath = NULL;

  /**
   * The TIF formatted surrogate from the JPG input.
   *
   * @var string
   */
  public $tifSurrogate = NULL;

  /**
   * Constructor.
   *
   * @param string $metadata_path
   *   The path to the metadata.php file to include.
   * @param string $target_path
   *   The already-existing path into which to write the output.
   */
  public function __construct($metadata_path, $target_path) {
    $this->metadataPath = $metadata_path;
    $this->targetPath = $target_path;
    $this->pagesDirectory = dirname($this->metadataPath);
    $this->setIssueConstants();
    $this->loadPageFiles();
  }

  /**
   * Announce a string to the user.
   *
   * @param string $string
   *   The string to announce.
   */
  private function announce($string) {
    print "[$this->curPageDir] " . $string . PHP_EOL;
  }

  /**
   * Announce the issue we are currently processing.
   */
  private function announceIssue() {
    $this->announce('Processing ' . ISSUE_TITLE . ':' . ISSUE_VOLUME . '/' . ISSUE_ISSUE);
    $this->announce('From ' . $this->metadataPath . PHP_EOL);
  }

  /**
   * Fix some common mistakes in the constant files.
   *
   * @param string $issue_metadata_constants_string
   *   The file contents of the metadata.php file to alter.
   *
   * @return string
   *   The fixed improved metadata.php contents.
   */
  private function fixIssueConstantsMistakes($issue_metadata_constants_string) {
    // Fix MS related quoting issues.
    $issue_metadata_constants_string = str_replace(
      array('‘', '’'),
      "'",
      $issue_metadata_constants_string
    );

    return $issue_metadata_constants_string;
  }

  /**
   * Generate and return the MODS file content for the issue.
   *
   * @return string
   *   The MODS file content.
   */
  private function generateIssueMods() {
    $issue_title_date_string = date('F j, Y', ISSUE_DATE);
    $issue_supp_title = ISSUE_SUPPLEMENT_TITLE;
    $issue_title = ISSUE_TITLE . ", $issue_title_date_string" . trim(" " . $issue_supp_title);
    $issue_date = date('Y-m-d', ISSUE_DATE);
    $issue_volume = ISSUE_VOLUME;
    $issue_issue = ISSUE_ISSUE;
    $issue_edition = ISSUE_EDITION;
    $issue_errata = ISSUE_ERRATA;
    $issue_missing = MISSING_PAGES;

    $issue_mods = <<<EOT
<?xml version="1.0" encoding="UTF-8"?>
<mods xmlns="http://www.loc.gov/mods/v3" xmlns:mods="http://www.loc.gov/mods/v3" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
    <titleInfo>
        <title>$issue_title</title>
        <subTitle />
    </titleInfo>
    <titleInfo type="alternative">
        <title>$issue_supp_title</title>
    </titleInfo>
    <part>
      <detail type="volume">
        <number>$issue_volume</number>
      </detail>
      <detail type="issue">
        <number>$issue_issue</number>
      </detail>
      <detail type="edition">
        <number>$issue_edition</number>
      </detail>
    </part>
    <note type="missingpages">$issue_missing</note>
    <note type="errata">$issue_errata</note>
    <name type="personal">
        <namePart type="given" />
        <namePart type="family" />
        <role>
            <roleTerm authority="marcrelator" type="text" />
        </role>
        <description />
    </name>
    <typeOfResource>text</typeOfResource>
    <genre authority="marcgt">newspaper</genre>
    <identifier type="issn" />
    <identifier type="hdl" />
    <identifier type="lccn" />
    <abstract />
    <note type="prospectus" />
    <originInfo>
        <publisher />
        <place>
            <placeTerm type="text" />
        </place>
        <dateIssued>$issue_date</dateIssued>
        <dateIssued point="start" />
        <dateIssued point="end" />
        <issuance>serial</issuance>
        <frequency authority="marcfrequency" />
    </originInfo>
    <subject>
        <topic />
    </subject>
    <subject>
        <geographic />
    </subject>
    <subject>
        <temporal />
    </subject>
    <relatedItem type="succeeding">
        <titleInfo>
            <title />
            <subTitle />
        </titleInfo>
        <originInfo>
            <publisher />
            <place>
                <placeTerm type="text" />
            </place>
            <issuance>continuing</issuance>
            <dateIssued />
            <dateIssued point="start" />
            <dateIssued point="end" />
            <frequency authority="marcfrequency" />
        </originInfo>
        <identifier type="issn" />
    </relatedItem>
</mods>
EOT;
    return $issue_mods;
  }

  /**
   * Generate the OCR surrogate.
   */
  private function generateHocr() {
    $this->announce('Generating HOCR');
    exec(
      'ulimit -t ' . NBNDPROCESSOR_TESSERACT_TIMEOUT . ' ; tesseract ' .
      $this->hocrSurrogate . ' ' .
      $this->curPageDir . '/HOCR -l ' . NBNDPROCESSOR_TESSERACT_LANGUAGE . ' hocr'
    );
  }

  /**
   * Copy the object file original.
   */
  private function generateHocrSurrogate() {
    $this->announce('Generating HOCR Surrogate');
    $temp_file = tempnam(sys_get_temp_dir(), 'HOCRsurrogate');
    exec(
      'gm convert ' .
      $this->curWorkFile .
      ' ' .
      NBNDPROCESSOR_HOCR_SURROGATE_ARGS .
      ' ' .
      $temp_file . '.tif'
    );
    $this->hocrSurrogate = $temp_file . '.tif';
  }

  /**
   * Generate the JP2 surrogate.
   *
   * Arguments were taken from defaults in islandora_solution_pack_large_image.
   */
  private function generateJp2() {
    $this->announce('Generating JP2');

    exec(
      'kdu_compress  -i ' . $this->tifSurrogate .
      ' -o ' . $this->curPageDir . '/JP2.jp2 ' .
      NBNDPROCESSOR_KAKADU_ARGS
    );
  }

  /**
   * Generate the JPEG surrogate.
   */
  private function generateJpeg() {
    $this->announce('Generating JPG');
    $this->resizeCrop(
      $this->curPageDir . '/JPG.jpg',
      NBNDPROCESSOR_ISLANDORA_JPG_SIZE,
      'North'
    );
  }

  /**
   * Copy the object file original.
   */
  private function generateObject() {
    $this->announce('Generating OBJ');
    exec(
      'cp ' .
      $this->curWorkFile .
      ' ' .
      $this->curPageDir . '/OBJ.jpg'
    );
  }

  /**
   * Generate the OCR surrogate.
   */
  private function generateOcr() {
    $this->announce('Distilling OCR from HOCR');
    $hocr_data = file_get_contents($this->curPageDir . '/HOCR.hocr');
    $hocr_xml = new DOMDocument();
    $is_loaded = $hocr_xml->loadXML($hocr_data);
    if ($is_loaded) {
      $raw_ocr = strip_tags($hocr_xml->saveHTML());
      file_put_contents($this->curPageDir . '/OCR.txt', $raw_ocr);
    }
  }

  /**
   * Copy the object file original.
   */
  private function generateTifSurrogate() {
    $this->announce('Generating TIFF Surrogate');
    $temp_file = tempnam(sys_get_temp_dir(), 'TIFsurrogate');
    exec(
      'convert ' .
      $this->curWorkFile .
      ' -compress none ' .
      $temp_file . '.tif'
    );
    $this->tifSurrogate = $temp_file . '.tif';
  }

  /**
   * Generate the TN surrogate.
   */
  private function generateTn() {
    $this->announce('Generating TN');
    $this->resizeCrop(
      $this->curPageDir . '/TN.jpg',
      NBNDPROCESSOR_ISLANDORA_TN_SIZE,
      'North'
    );
  }

  /**
   * Load page files and numbers into our final processing array.
   */
  private function loadPageFiles() {
    $directory = new DirectoryIterator($this->pagesDirectory);
    $flattened = new IteratorIterator($directory);
    $files = new RegexIterator($flattened, '/^[^.].*\.jpg$/i');

    // Get files in page order.
    $page_files = array();
    foreach ($files as $file) {
      $page_files[] = $file->getFilename();
    }
    sort($page_files);

    // Stuff page array.
    foreach ($page_files as $page_file) {
      $file_basename = basename($page_file, ".jpg");
      $file_name_data = explode('_', $file_basename);
      $page_number = (int) array_pop($file_name_data);
      $page_number_styled = $this->stylePageNumber($page_number);
      $this->pages[$page_number_styled] = $page_file;
    }
  }

  /**
   * Process the files and write them and metadata to the target directory.
   */
  public function process() {
    $this->announceIssue();

    // Generate title MODS.
    file_put_contents(
      $this->targetPath . '/MODS.xml',
      $this->generateIssueMods()
    );

    // Generate pages.
    $cur_page = 1;
    foreach ($this->pages as $page_number => $page_file) {
      if ($this->stylePageNumber($page_number) == $this->stylePageNumber($cur_page)) {
        $page_dir = $this->targetPath . '/' . $this->stylePageNumber($cur_page);
        exec('mkdir -p ' . $page_dir);
        $this->curPageDir = $page_dir;
        $this->curWorkFile = "$this->pagesDirectory/$page_file";
        $this->generateObject();
        $this->generateTifSurrogate();
        $this->generateHocrSurrogate();
        $this->generateHocr();
        $this->generateOcr();
        $this->generateJp2();
        $this->generateJpeg();
        $this->generateTn();
        $this->removeSurrogates();
      }
      else {
        while ($cur_page < (int) $page_number) {
          echo "Missing Page " . $this->stylePageNumber($cur_page) . PHP_EOL;
          $page_dir = $this->targetPath . '/' . $this->stylePageNumber($cur_page);
          exec('mkdir -p ' . $page_dir);
          exec('cp ' . dirname(__FILE__) . '/missing/* ' . $page_dir);
          $cur_page++;
        }
      }
      $cur_page++;
    }

  }

  /**
   * Setup and evaluate the issue constants.
   */
  private function removeSurrogates() {
    $this->announce('Removing Surrogates');
    unlink($this->tifSurrogate);
    unlink($this->hocrSurrogate);
  }

  /**
   * Resize and crop an image using imagemagick.
   *
   * @param string $output_file
   *   The output file.
   * @param string $dimensions
   *   The dimensions to crop to, in YYYxYYY format.
   * @param string $float
   *   The Imagemagick gravity setting for the crop.
   */
  private function resizeCrop($output_file, $dimensions, $float) {
    exec(
      'convert ' .
      $this->curWorkFile .
      ' -resize "' . $dimensions . '^" -gravity ' . $float . ' -crop ' . $dimensions . '+0+0 +repage ' .
      $output_file
    );
  }

  /**
   * Setup and evaluate the issue constants.
   */
  private function setIssueConstants() {
    $issue_metadata_constants_string = file_get_contents($this->metadataPath);
    eval($this->fixIssueConstantsMistakes($issue_metadata_constants_string));
  }

  /**
   * Style the page number format into a standard format.
   *
   * @param string $page_number
   *   The image number to style into the format.
   *
   * @return string
   *   The formatted page number.
   */
  private function stylePageNumber($page_number) {
    return str_pad($page_number, NBNDPROCESSOR_ISSUE_NUMBER_LENGTH, "0", STR_PAD_LEFT);
  }

}
