# Screenshots (text captures)

These files were produced with the headless command of the program itself, e.g.

    build/algoviz.exe --dump bubble_sort --values 5,2,8,1,4 --step 6 --code --theme 4 --width 120 --height 32

`--theme 4` is the **monochrome** theme (no colour; meaning is carried by `[CMP]`/`[SWP]`/... tags), `--theme 0` is Dark.
File 10 contains the raw ANSI escape sequences of the Dark theme (open it with `type`/`cat` in a terminal to see colours).
For the report, take real colour screenshots of the live program (`build/algoviz.exe`) in each theme: press `T` to
cycle themes (Dark, Light, High contrast, Color-blind safe, Monochrome), `U` to switch Unicode/ASCII, `H` for help.

| File | Shows |
|---|---|
| 01_bubble_sort_mono_code.txt | step view + information panel + code view (monochrome) |
| 02_binary_search_mono.txt | search range, pointers, eliminated range |
| 03_merge_sort_temp_row.txt | merge sort with its temporary row |
| 04_string_sort.txt | string sorting, character-by-character explanation |
| 05_linked_list_insert.txt | linked list: new node not yet linked, pointer labels |
| 06_hash_linear_probing.txt | probe, collision, statistics |
| 07a / 07b_avl_*.txt | AVL tree: node 10 unbalanced (bf=-2, RR case), then after the left rotation |
| 08_heap_tree_and_array.txt | heap as tree and array |
| 09_graph_bfs_drawing.txt | graph drawing during BFS with the queue |
| 10_dark_theme_ansi_codes.txt | Dark theme with ANSI codes |
| 11_stack.txt / 12_circular_queue_wrap.txt | stack with TOP, circular queue after wrap-around |
| 13_summary_report_example.md | the exported Markdown report |

Still to capture by hand (needs a real terminal): Race Mode split view, Quiz prompt and score screens, the home menu, the
Color-blind and High-contrast themes, a custom (plugin) algorithm, the help dialog.
