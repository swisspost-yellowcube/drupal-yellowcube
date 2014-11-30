<?php
/**
 * @file
 * Cron tasks for the YellowCube module.
 */

/**
 * Cron callback for updating product status.
 */
function yellowcube_jobs_update_products() {
  $queue = DrupalQueue::get('yellowcube_product_status');

  // Select products that either have never been sent to YellowCube or have been
  // changed since they were. Also include any products that were queued more
  // than 6 hours ago, since their processing is then assumed to have failed.
  $update_candidate = db_query("
    SELECT cp.product_id, sku, cp.title, cp.type, cp.created, cp.changed,
           yps.updated AS status_updated, yps.queued
    FROM {commerce_product} AS cp
    LEFT JOIN {yellowcube_product_status} AS yps USING (sku)
    WHERE (yps.article_status = 'enqueued' AND yps.queued < :previous)
          OR yps.article_status IS NULL OR yps.updated < cp.changed
  ", array(':previous' => REQUEST_TIME - 6 * 3600));

  foreach ($update_candidate as $up_cand) {
    $queue->createItem($up_cand->sku);

    db_merge('yellowcube_product_status')
      ->key(array('sku' => $up_cand->sku))
      ->fields(array(
        'article_status' => 'enqueued',
        'queued' => REQUEST_TIME,
      ))
      ->execute();
  }
}
