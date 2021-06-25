/*******************************************************************\
 * csv-gdpdu-export.c -- Export data for German tax authority       *
 *                                                                  *
 * Copyright (C) 2012 Robert Fewell                                 *
 * Copyright (C) 2021 Alexander Graf                                *
 *                                                                  *
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652       *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                   *
\********************************************************************/
/** @file csv-gdpdu-export.h
    @brief GDPdU data set export for German tax authority
    @author Copyright (c) 2012 Robert Fewell
    @author Copyright (c) 2021 Alexander Graf
*/
#include "config.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "gnc-commodity.h"
#include "gnc-ui-util.h"
#include "Query.h"
#include "Transaction.h"
#include "qofbookslots.h"

#include "csv-gdpdu-export.h"

/* This static indicates the debugging module that this .o belongs to. */
static QofLogModule log_module = GNC_MOD_ASSISTANT;

/* CSV spec requires CRLF line endings. Tweak the end-of-line string so this
 * true for each platform */
#ifdef G_OS_WIN32
# define EOLSTR "\n"
#else
# define EOLSTR "\r\n"
#endif


/*******************************************************
 * write_line_to_file
 *
 * write a text string to a file pointer, return TRUE if
 * successful.
 *******************************************************/
static
gboolean write_line_to_file (FILE *fh, char *line)
{
  int len, written;
  DEBUG("Account String: %s", line);

  /* Write account line */
  len = strlen (line);
  written = fwrite (line, 1, len, fh);

  if (written != len)
    return FALSE;
  else
    return TRUE;
}

/*******************************************************
 * csv_txn_test_field_string
 *
 * Test the field string for ," and new lines
 *******************************************************/
static
gchar *csv_txn_test_field_string (CsvExportInfo *info, const gchar *string_in)
{
  gboolean need_quote = FALSE;
  gchar **parts;
  gchar *string_parts;
  gchar *string_out;

  /* Check for " and then "" them */
  parts = g_strsplit (string_in, "\"", -1);
  string_parts = g_strjoinv ("\"\"", parts);
  g_strfreev (parts);

  /* Check for separator string and \n and " in field,
     if so quote field if not already quoted */
  if (g_strrstr (string_parts, info->separator_str) != NULL)
    need_quote = TRUE;
  if (g_strrstr (string_parts, "\n") != NULL)
    need_quote = TRUE;
  if (g_strrstr (string_parts, "\"") != NULL)
    need_quote = TRUE;

  if (!info->use_quotes && need_quote)
    string_out = g_strconcat ("\"", string_parts, "\"", NULL);
  else
    string_out = g_strdup (string_parts);

  g_free (string_parts);
  return string_out;
}

/******************** Helper functions *********************/

static gchar *
write_account_guid (gchar *so_far, Account *account, CsvExportInfo *info)
{
  gchar *result;
  gchar *guid;

  guid = guid_to_string (xaccAccountGetGUID (account));
  result = g_strconcat (so_far, guid, info->mid_sep, NULL);
  g_free (guid);
  g_free (so_far);
  return result;
}

static gchar *
write_account_parent_guid (gchar *so_far, Account *account, CsvExportInfo *info)
{
  gchar *result;
  gchar *guid;
  Account *parent;

  if (gnc_account_is_root (account))
  {
    result = g_strconcat (so_far, info->mid_sep, NULL);
    g_free (so_far);
    return result;
  }

  parent = gnc_account_get_parent (account);

  if (gnc_account_is_root (parent))
  {
    result = g_strconcat (so_far, info->mid_sep, NULL);
  }
  else
  {
    guid = guid_to_string (xaccAccountGetGUID (parent));
    result = g_strconcat (so_far, guid, info->mid_sep, NULL);
    g_free (guid);
  }

  g_free (so_far);
  return result;
}

static gchar *
write_account_code (gchar *so_far, Account *account, CsvExportInfo *info)
{
  gchar *result;
  const gchar *code;
  gchar *conv;

  code = xaccAccountGetCode (account) ? xaccAccountGetCode (account) : "";
  conv = csv_txn_test_field_string (info, code);
  result = g_strconcat (so_far, conv, info->mid_sep, NULL);
  g_free (conv);
  g_free (so_far);
  return result;
}

static gchar *
write_account_name (gchar *so_far, Account *account, gboolean full, CsvExportInfo *info)
{
  gchar *name = NULL;
  gchar *conv;
  gchar *result;

  if (full)
    name = gnc_account_get_full_name (account);
  else
    name = g_strdup (xaccAccountGetName (account));
  conv = csv_txn_test_field_string (info, name);
  result = g_strconcat (so_far, conv, info->mid_sep, NULL);
  g_free (name);
  g_free (conv);
  g_free (so_far);
  return result;
}

static gchar *
write_transaction_guid (gchar *so_far, Transaction *trans, CsvExportInfo *info)
{
  gchar *result;
  gchar *guid;

  guid = guid_to_string (xaccTransGetGUID (trans));
  result = g_strconcat (so_far, guid, info->mid_sep, NULL);
  g_free (guid);
  g_free (so_far);
  return result;
}

static gchar *
write_transaction_reversedby_guid (gchar *so_far, Transaction *trans, CsvExportInfo *info)
{
  gchar *result;
  gchar *guid;
  Transaction *reversedBy;

  reversedBy = xaccTransGetReversedBy (trans);
  if (reversedBy)
  {
    guid = guid_to_string (xaccTransGetGUID (reversedBy));
    result = g_strconcat (so_far, guid, info->mid_sep, NULL);
    g_free (guid);
  }
  else
  {
    result = g_strconcat (so_far, info->mid_sep, NULL);
  }
  g_free (so_far);
  return result;
}

static gchar *
write_transaction_date_posted (gchar *so_far, Transaction *trans, CsvExportInfo *info)
{
  gchar *date = qof_print_date (xaccTransGetDate (trans));
  gchar *result = g_strconcat (so_far, date, info->mid_sep, NULL);
  g_free (date);
  g_free (so_far);
  return result;
}

static gchar *
write_transaction_date_entered (gchar *so_far, Transaction *trans, CsvExportInfo *info)
{
  gchar *date = qof_print_date (xaccTransGetDateEntered (trans));
  gchar *result = g_strconcat (so_far, date, info->mid_sep, NULL);
  g_free (date);
  g_free (so_far);
  return result;
}

static gchar *
write_transaction_number (gchar *so_far, Transaction *trans, CsvExportInfo *info)
{
  const gchar *num;
  gchar *conv;
  gchar *result;

  num = xaccTransGetNum (trans) ? xaccTransGetNum (trans) : "";
  conv = csv_txn_test_field_string (info, num);
  result = g_strconcat (so_far, conv, info->mid_sep, NULL);
  g_free (conv);
  g_free (so_far);
  return result;
}

static gchar *
write_transaction_description (gchar *so_far, Transaction *trans, CsvExportInfo *info)
{
  const gchar *desc;
  gchar *conv;
  gchar *result;

  desc = xaccTransGetDescription (trans) ? xaccTransGetDescription (trans) : "";
  conv = csv_txn_test_field_string (info, desc);
  result = g_strconcat (so_far, conv, info->mid_sep, NULL);
  g_free (conv);
  g_free (so_far);
  return result;
}

static gchar *
write_transaction_doclink (gchar *so_far, Transaction *trans, CsvExportInfo *info)
{
  const gchar *desc;
  gchar *conv;
  gchar *result;

  desc = xaccTransGetDocLink (trans) ? xaccTransGetDocLink (trans) : "";
  conv = csv_txn_test_field_string (info, desc);
  result = g_strconcat (so_far, conv, info->mid_sep, NULL);
  g_free (conv);
  g_free (so_far);
  return result;
}

static gchar *
write_split_guid (gchar *so_far, Split *split, CsvExportInfo *info)
{
  gchar *result;
  gchar *guid;

  guid = guid_to_string (xaccSplitGetGUID (split));
  result = g_strconcat (so_far, guid, info->mid_sep, NULL);
  g_free (guid);
  g_free (so_far);
  return result;
}

static gchar *
write_split_action (gchar *so_far, Split *split, CsvExportInfo *info)
{
  const gchar *action = xaccSplitGetAction (split);
  gchar *conv = csv_txn_test_field_string (info, action);
  gchar *result = g_strconcat (so_far, conv, info->mid_sep, NULL);
  g_free (conv);
  g_free (so_far);
  return result;
}

static gchar *
write_split_memo (gchar *so_far, Split *split, CsvExportInfo *info)
{
  const gchar *memo;
  gchar *conv;
  gchar *result;

  memo = xaccSplitGetMemo (split) ? xaccSplitGetMemo (split) : "";
  conv = csv_txn_test_field_string (info, memo);
  result = g_strconcat (so_far, conv, info->mid_sep, NULL);
  g_free (conv);
  g_free (so_far);
  return result;
}

static gchar *
write_split_amount (gchar *so_far, Split *split, gboolean t_void, gboolean symbol, CsvExportInfo *info)
{
  const gchar *amt;
  gchar *conv;
  gchar *result;

  if (t_void)
    amt = xaccPrintAmount (xaccSplitVoidFormerAmount (split), gnc_split_amount_print_info (split, symbol));
  else
    amt = xaccPrintAmount (xaccSplitGetAmount (split), gnc_split_amount_print_info (split, symbol));
  conv = csv_txn_test_field_string (info, amt);
  result = g_strconcat (so_far, conv, info->mid_sep, NULL);
  g_free (conv);
  g_free (so_far);
  return result;
}

static gchar *
write_end_seperator (gchar *so_far, CsvExportInfo *info)
{
  gchar *result;

  result = g_strconcat (so_far, EOLSTR, NULL);
  g_free (so_far);
  return result;
}

/******************************************************************************/

static
void write_splits_table (CsvExportInfo *info, FILE *fh)
{
  GSList *p1, *p2;
  GList *splits;
  QofBook *book;
  Query *query;

  // Setup the query for normal split export
  query = qof_query_create_for (GNC_ID_SPLIT);
  book = gnc_get_current_book ();
  qof_query_set_book (query, book);

  /* Sort by transaction date */
  p1 = g_slist_prepend (NULL, TRANS_DATE_POSTED);
  p1 = g_slist_prepend (p1, SPLIT_TRANS);
  p2 = g_slist_prepend (NULL, QUERY_DEFAULT_SORT);
  qof_query_set_sort_order (query, p1, p2, NULL);

  /* Run the query */
  for (splits = qof_query_run (query); splits; splits = splits->next)
  {
    Split *split;
    Transaction *transaction;
    Account *account;
    gchar *line;
    gboolean t_void;

    split = splits->data;
    transaction = xaccSplitGetParent (split);
    account = xaccSplitGetAccount (split);

    // Look for blank split
    if (xaccSplitGetAccount (split) == NULL)
      continue;

    t_void = xaccTransGetVoidStatus (transaction);
    line = g_strdup ("");
    line = write_split_guid (line, split, info);
    line = write_transaction_guid (line, transaction, info);
    line = write_account_guid (line, account, info);
    line = write_split_amount (line, split, t_void, FALSE, info);
    line = write_split_action (line, split, info);
    line = write_split_memo (line, split, info);
    line = write_end_seperator (line, info);

    if (!write_line_to_file (fh, line))
    {
      info->failed = TRUE;
      break;
    }
    g_free (line);
  }
  qof_query_destroy (query);
  g_list_free (splits);
}

static
void write_transactions_table (CsvExportInfo *info, FILE *fh)
{
  GSList *p1, *p2;
  GList *transactions;
  QofBook *book;
  Query *query;

  // Setup the query for normal transaction export
  query = qof_query_create_for (GNC_ID_TRANS);
  book = gnc_get_current_book ();
  qof_query_set_book (query, book);

  /* Sort by transaction date posted */
  p1 = g_slist_prepend (NULL, TRANS_DATE_POSTED);
  p2 = g_slist_prepend (NULL, QUERY_DEFAULT_SORT);
  qof_query_set_sort_order (query, p1, p2, NULL);

  /* Run the query */
  for (transactions = qof_query_run (query); transactions; transactions = transactions->next)
  {
    Transaction *transaction;
    gchar *line;
    gboolean t_void;

    transaction = transactions->data;

    t_void = xaccTransGetVoidStatus (transaction);
    line = g_strdup ("");
    line = write_transaction_guid (line, transaction, info);
    line = write_transaction_reversedby_guid (line, transaction, info);
    line = write_transaction_date_posted (line, transaction, info);
    line = write_transaction_date_entered (line, transaction, info);
    line = write_transaction_number (line, transaction, info);
    line = write_transaction_description (line, transaction, info);
    line = write_transaction_doclink (line, transaction, info);
    line = write_end_seperator (line, info);

    if (!write_line_to_file (fh, line))
    {
      info->failed = TRUE;
      break;
    }
    g_free (line);
  }
  qof_query_destroy (query);
  g_list_free (transactions);
}

static
void write_accounts_table (CsvExportInfo *info, FILE *fh)
{
  Account *root;
  GList *accounts;

  root = gnc_book_get_root_account (gnc_get_current_book ());

  /* Run the query */
  for (accounts = gnc_account_get_descendants_sorted (root); accounts; accounts = accounts->next)
  {
    Account *account;
    gchar *line;
    gboolean t_void;

    account = accounts->data;

    line = g_strdup ("");
    line = write_account_guid (line, account, info);
    line = write_account_parent_guid (line, account, info);
    line = write_account_code (line, account, info);
    line = write_account_name (line, account, FALSE, info);
    line = write_account_name (line, account, TRUE, info);
    line = write_end_seperator (line, info);

    if (!write_line_to_file (fh, line))
    {
      info->failed = TRUE;
      break;
    }
    g_free (line);
  }
  g_list_free (accounts);
}

static
void csv_gdpdu_export_splits (CsvExportInfo *info)
{
  FILE *fh;
  gchar *splits_file_name;
  splits_file_name = g_strconcat (info->file_name, "_splits.csv", NULL);

  ENTER("");
  DEBUG("File name is : %s", splits_file_name);

  fh = g_fopen (splits_file_name, "w");
  if (fh != NULL)
  {
    write_splits_table (info, fh);
    g_list_free (info->trans_list);
  }
  else
    info->failed = TRUE;
  if (fh)
    fclose (fh);
  g_free (splits_file_name);
  LEAVE("");
}

static
void csv_gdpdu_export_transactions (CsvExportInfo *info)
{
  FILE *fh;
  gchar *splits_file_name;
  splits_file_name = g_strconcat (info->file_name, "_transactions.csv", NULL);

  ENTER("");
  DEBUG("File name is : %s", splits_file_name);

  fh = g_fopen (splits_file_name, "w");
  if (fh != NULL)
  {
    write_transactions_table (info, fh);
    g_list_free (info->trans_list);
  }
  else
    info->failed = TRUE;
  if (fh)
    fclose (fh);
  g_free (splits_file_name);
  LEAVE("");
}

static
void csv_gdpdu_export_accounts (CsvExportInfo *info)
{
  FILE *fh;
  gchar *splits_file_name;
  splits_file_name = g_strconcat (info->file_name, "_accounts.csv", NULL);

  ENTER("");
  DEBUG("File name is : %s", splits_file_name);

  fh = g_fopen (splits_file_name, "w");
  if (fh != NULL)
  {
    write_accounts_table (info, fh);
    g_list_free (info->trans_list);
  }
  else
    info->failed = TRUE;
  if (fh)
    fclose (fh);
  g_free (splits_file_name);
  LEAVE("");
}

void csv_gdpdu_export (CsvExportInfo *info)
{
  info->failed = FALSE;

  info->end_sep = "\"";
  info->separator_str = ";";
  info->mid_sep = g_strconcat (";", NULL);

  csv_gdpdu_export_splits (info);
  csv_gdpdu_export_transactions (info);
  csv_gdpdu_export_accounts (info);
}
