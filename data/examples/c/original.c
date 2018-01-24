static void
complete_highlight_groups(const char str[])
{
  const size_t len = strlen(str);

  col_scheme_t *const cs = curr_stats.cs;
  for(int i = 0; i < cs->file_hi_count; ++i)
  {
    const char *const expr =
        ma_get_expr(cs->file_hi[i].matchers);
    if(strncasecmp(str, expr, len) == 0)
    {
      vle_compl_add_match(expr, "");
    }
  }

  if(!file_hi_only)
  {
    for(int i = 0; i < MAXNUM_COLOR; ++i)
    {
      if(strncasecmp(str, GROUPS[i], len) == 0)
      {
        vle_compl_add_match(GROUPS[i],
                            GROUPS_DESCR[i]);
      }
    }

    if(strncmp(str, "clear", len) == 0)
    {
      vle_compl_add_match("clear",
                          "clear color rules");
    }
  }

  vle_compl_finish_group();
  vle_compl_add_last_match(str);
}
