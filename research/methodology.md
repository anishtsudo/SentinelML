# Experimental Methodology

Status: **proposed**. Source tags (S1–S6) refer to [dataset_analysis.md](dataset_analysis.md).
Nothing here has been measured yet. Every number is either cited or a
back-of-envelope estimate, and estimates are labelled as such.

## Guiding principles

1. **The test set represents unseen traffic.** Nothing fitted on test data: not
   scalers, feature selection, thresholds, hyperparameters or early stopping
   (S6: data snooping, biased parameter selection).
2. **Report per-class and operating-point metrics, never only aggregates.** In S3,
   weighted F1 stayed at 0.99 while per-class F1 for XSS and Bot moved by 0.4–0.6.
3. **Identifiers are metadata, not features.** Flow ID, IPs, source port and
   timestamp are kept for splitting and analysis, and are never model inputs.
4. **Every number traces to code, a config, a data version and an environment report.**

---

## 1. Split strategies compared

### A. Random stratified split (e.g. 60/20/20 per class)
- **What it is:** rows are shuffled, and each class is split in proportion.
- **For:**
  - Every class appears in every split.
  - It's simple and matches most published CICIDS2017 results, so it's useful for
    comparing against the literature.
- **Against:**
  - Flows from the same attack run are near-identical: one tool, one target,
    seconds apart. A random split puts siblings on both sides, so the test measures
    memorisation of that run, not detection (S6: data snooping).
  - With the original CICFlowMeter, TCP appendices of one connection can be split
    across train and test (S3).
  - Training includes data from *after* test events (temporal snooping, S6).
- **Verdict:** optimistically biased. Use only as a **reference** to measure how much
  it inflates results.

### B. Split by capture day (e.g. train Mon–Wed, test Thu–Fri)
- **What it is:** whole days go into train or test, the most deployment-like setup.
- **Fatal problem for supervised learning:** **each attack type occurs on exactly
  one day** (S1).
  - Training on Mon–Wed sees only brute force, DoS and Heartbleed.
  - Testing on Thu–Fri contains only web attacks, infiltration, bot, port scan and
    DDoS, none of which were seen in training.
  - Multi-class evaluation is impossible, and binary evaluation becomes
    "detect unseen attack families", confounded with day-to-day benign drift.
- **Where it *is* useful:** Monday is attack-free (S1). That makes it a natural clean
  training period for **anomaly detectors** (Isolation Forest, autoencoder), which
  learn "normal" without labels.
- **Verdict:** not viable as the primary supervised split. Kept as the
  anomaly-detection protocol (E4 below).

### C. Holding out attack categories (leave-one-family-out)
- **What it is:** train with one attack family removed entirely, then test whether
  the model still flags that family as malicious.
- **For:** this is the question that matters in practice ("does it catch attacks it
  wasn't trained on?"), and the fair test for anomaly detectors.
- **Against:**
  - It answers a different question from "how well does it classify known attacks".
  - Results depend on which family is held out.
  - Tiny families (Heartbleed: 11 flows in S3 Table I) give very noisy estimates.
- **Verdict:** essential, but as a **generalisation experiment**, not the main
  benchmark.

### Recommended primary: temporally blocked split within each attack episode

None of the three works as the main experiment, so we propose this:

1. Define an **episode** as all flows of one class on one day. Benign is one episode
   per day.
2. Within each episode, sort by flow start time and cut by count:
   **first 60% → train, next 20% → validation, last 20% → test.**
3. **Embargo:** discard flows starting within **120 s** of a split boundary. 120 s is
   the CICIDS2017 flow timeout (S3), so one long connection cut into consecutive
   flows can't land on both sides of a boundary.
4. Remove exact duplicates *before* splitting (§3.7). Then report how many test
   vectors still appear identically in training. This should be zero by construction.

**Why this is the best available option:**
- Every class (above a size threshold) appears in train, val and test, so supervised
  multi-class evaluation works.
- The model is always tested on *later* traffic of an attack than it trained on.
  This breaks the near-duplicate-sibling leakage of option A.
- It's deterministic and needs no random seed.

**Limitation, to state in every report:** this is still a *within-campaign* test.
Train and test come from the same tool run, against the same victim, on the same
day. It measures "detecting the continuation of a known attack", not "detecting the
attack class in the wild". E3 and the cross-dataset experiment cover the latter.

**[VERIFY]** before adopting:
- The chosen dataset version has per-flow timestamps (§2, decision D-A).
- How many flows the embargo removes per class.
- Whether the tiny classes have usable test sets.

### Experiment matrix

| ID | Experiment | Split | Models | Question |
|---|---|---|---|---|
| **E1** | **Primary detection benchmark** | Temporal block per episode | LR, RF, XGBoost, MLP | Which model detects known attacks best, and at what cost? |
| E2 | Literature reference | Random stratified 60/20/20 | Same as E1 | How much does random splitting inflate results? (The E2 − E1 gap is a finding) |
| E3 | Unseen-attack generalisation | Leave-one-family-out on E1's splits | Binary RF / XGBoost / MLP | Does a supervised model flag attack families it never saw? |
| E4 | Anomaly detection | Train on benign only (Monday + benign train blocks); test on E1 test | Isolation Forest, autoencoder | How do unsupervised detectors compare, without attack labels? |
| E5 | Cross-dataset | Train CICIDS2017, test UNSW-NB15 (and reverse) | Best of E1 | Do models generalise across networks? (Blocked on decision D-G) |
| E6 | Feature ablations | E1 | Best of E1 | ± Destination Port; ± fingerprint-like features (Init Win bytes, header lengths) |

### Metrics

- **Binary (benign vs attack):**
  - PR-AUC as the primary metric. It accounts for imbalance, unlike ROC-AUC.
  - ROC-AUC.
  - **Detection rate at fixed false-positive rates of 1% and 0.1%**, the numbers an
    operator cares about.
  - F1 at a threshold chosen on validation.
- **Multi-class:**
  - per-class precision, recall and F1;
  - macro-F1, the headline figure, since it weights each class equally;
  - the confusion matrix.
- **Uncertainty:**
  - bootstrap 95% CIs on test metrics;
  - 5 training seeds for stochastic models;
  - classes with fewer than about 30 test flows are reported with counts and
    qualitatively, not as F1.
- **Sanity checks:**
  - feature importance on each final model (S3's recommendation against shortcut
    learning);
  - per-host error breakdown to catch IP-fingerprint leakage.

---

## 2. Dataset version (decision D-A)

| Option | Pros | Cons |
|---|---|---|
| Original `MachineLearningCSV` / `GeneratedLabelledFlows` (S1) | What most papers use; directly comparable | About 26% of flows are TCP-appendix artefacts and >20% of traces are mislabelled (S3). Results partly measure tool bugs |
| WTMC2021 regenerated (S3w) | Fixed flow construction; payload-based relabelling | Superseded by CNS2022 label fixes |
| **CNS2022 "improved" (S4)** | Most complete label corrections; labelling logic released; 328 MB download | Less directly comparable to older papers. **Columns not yet verified** |

**Recommendation:**
- Use **CNS2022 improved** for E1 and E3–E6. Its labels are the most defensible.
- Use the **original** release only for E2, to reproduce literature-style numbers and
  quantify how much the known errors change conclusions. That comparison is a
  publishable finding in itself.

If the improved release lacks per-flow timestamps, the temporal split falls back to
the WTMC2021 version or `GeneratedLabelledFlows`.

For the inference-performance research questions, the version barely matters: the
feature dimensionality is about the same.

---

## 3. Memory-efficient data pipeline (16 GB M4)

### 3.1 Budget (estimates; to be measured)

- About 2.8 M rows × about 80 numeric columns (S3).
  - float64: 2.8e6 × 80 × 8 B ≈ **1.8 GB**.
  - float32: ≈ **0.9 GB**.
- A naive `pandas.read_csv` creates float64, plus Python-object strings for
  identifiers, plus temporary copies during cleaning. That can plausibly reach
  several GB.
- **Target: peak RSS under 4 GB for any pipeline stage.** That leaves room for the
  OS, the IDE and model training. The pipeline records peak RSS with `psutil`, so
  this is checked rather than assumed.

### 3.2 Stages

```
data/raw/<version>/           untouched download + SHA-256 manifest (committed)
        │  stage 1: profile   (streaming, read-only → results/profile_<version>.json; curated summary → research/)
        │  stage 2: ingest    (per file → clean typed Parquet)
        ▼
data/processed/<version>/flows.parquet     features float32 + metadata columns
        │  stage 3: dedup + split assignment (adds `split` column, writes split manifest)
        ▼
model-ready float32 matrices (built on demand, train/val/test)
```

1. **Profile before cleaning.** One streaming pass over every CSV with
   `pyarrow.csv.open_csv` in record batches, so the whole file is never in memory.
   - Per column: inferred type, NaN, +Inf, -Inf, negative, zero, min/max, distinct
     count (capped).
   - Per file: row count and label counts.
   - Raw label strings and their byte sequences, to catch encoding problems.

   Every cleaning rule in stage 2 is justified by a number from this report, which
   resolves all the [VERIFY] items in dataset_analysis.md.
2. **Ingest one file at a time** into a typed table:
   - Explicit schema: no type inference on the full data.
   - Column names normalised: whitespace stripped, snake_case, through an explicit
     mapping. An unknown column fails loudly.
   - The duplicated `Fwd Header Length` is checked to be identical to the original
     column, then dropped.
   - Timestamps parsed into `int64` microseconds, once the format and timezone are
     confirmed by profiling.
3. **Dedup and split** operate on the processed Parquet. Only the columns they need
   are read: the feature-hash inputs, the timestamp and the label.

### 3.3 Numeric types

- **Features are stored as float32.**
  - XGBoost and scikit-learn trees convert inputs to float32 internally, and our C++
    engine will use float32. So float32 is exactly what the models see, and storing
    float64 would only double memory for precision the models discard.
- **Precision check:**
  - float32 represents integers exactly only up to 2^24 ≈ 16.7 M.
  - `Flow Duration` (µs) reaches 120 s = 1.2e8 µs, where float32 spacing is 8 µs.
    That is negligible for detection, but profiling records the max of every column
    and flags any column where conversion changes values materially.
  - Conversion to float32 can also create new duplicates (§3.7); we count those
    separately.
- **Metadata stays in its native type:**
  - IPs as dictionary-encoded strings;
  - ports as `uint16`;
  - protocol as `uint8`;
  - timestamp as `int64`;
  - label as a dictionary or categorical column.

### 3.4 Avoiding copies

- Use Arrow end to end, and convert to NumPy only at the model boundary.
- Build the model matrix as **one preallocated C-contiguous `float32 (n, d)` array**,
  filled column by column. Avoid `pd.concat` and chains of DataFrames. Row-major
  layout is also what the C++ inference engine will consume, so the Python and C++
  benchmarks share the same memory layout.
- Rely on pandas ≥ 3 copy-on-write semantics, and avoid chained assignment.
- Process per file and release memory with `del` before moving on. Measure peak RSS
  per stage.

### 3.5 Categorical features

- CICFlowMeter's behavioural features are all numeric.
- **Protocol** (TCP/UDP/other) is the only behavioural categorical:
  - raw integer code for tree models;
  - one-hot encoding for LR and MLP.
- **Destination port** is excluded from the primary feature set (dataset_analysis
  §5.4) and tested in ablation E6.
- IPs, flow ID, source port and timestamp: metadata only.

### 3.6 Missing and infinite values

The policy is finalised after profiling. Proposed:

1. Convert ±Inf to NaN, and count both per column **and per class**. A NaN rate that
   differs by class is itself a shortcut a model can learn.
2. If NaN/Inf come only from rate features on zero-duration flows (suspected, not
   verified):
   - set those rates to 0;
   - add a binary `zero_duration` feature, so the information is kept explicitly
     rather than through a magic value.
3. Drop any remaining NaN rows and report how many, per class. S3w also excluded
   NaN entries.
4. Constant columns: identify them **on the training split only**, then drop them.
5. Negative values: keep unless the profile shows they are sentinels (e.g. -1 for
   "not applicable"). Sentinels get the same value-plus-indicator treatment.
6. LR, MLP and autoencoder use a scaler fitted on the training split only. Heavy
   tails such as byte counts may need `log1p` first; decide after profiling.

### 3.7 Duplicates

- Compute a 64-bit hash per row over the **float32 feature bytes** (what the model
  sees). For 2.8 M rows that is about 22 MB. Hashes of all files fit in memory, so
  global dedup never needs all the features in memory at once.
- Report three counts, per class:
  1. **exact duplicates**: same features, same label;
  2. **conflicting duplicates**: same features, different labels;
  3. duplicates created by the float32 conversion.
- Proposed policy:
  - keep the earliest occurrence of each exact duplicate;
  - **drop every member of a conflicting group**, because the label is unknowable
    from features alone;
  - record all removals in a manifest.
- Dedup happens **before** splitting, so no test vector appears in training.
- Trade-off to state in reports: deduplication changes class priors. DoS and scan
  classes shrink the most, so results describe "distinct flow patterns", not raw
  traffic volume.

### 3.8 Label normalisation

- Use an explicit mapping table from every raw label string, as observed in
  profiling including encoding quirks, to:
  - a canonical class name;
  - a coarse family: Benign, DoS, DDoS, PortScan, BruteForce, WebAttack, Bot,
    Infiltration, Heartbleed.
- An unmapped label fails the pipeline.
- `Attempted` flows (S3/S4) are mapped according to decision D-B. They are never
  their own class (S4).

### 3.9 Testing the pipeline

Use small synthetic CSV fixtures that reproduce each known quirk:
- leading-space headers;
- the duplicated column;
- Inf/NaN;
- negative sentinels;
- mis-encoded labels;
- exact and conflicting duplicates.

Test that cleaning, dedup counts, label mapping and split assignment (including the
embargo) behave as specified. Tests never need the real dataset.

---

## 4. Unresolved decisions

| ID | Decision | Recommendation | Needs |
|---|---|---|---|
| D-A | Dataset version | CNS2022 improved for E1/E3–E6; original only for E2 | Verify columns and timestamps after download |
| D-B | `Attempted` flows | Relabel as Benign (authors' guidance, S4); sensitivity run with them dropped | — |
| D-C | DoS Hulk (ineffective attack, S3) | Keep in primary run (S4 keeps the label); ablation without it | — |
| D-D | Task granularity / tiny classes | Binary + coarse-family multi-class; classes with <30 test flows reported qualitatively; Heartbleed and SQLi used mainly in E3 | Per-class counts after dedup and embargo |
| D-E | Destination port | Exclude; ablation E6 | — |
| D-F | Duplicate policy | Keep earliest exact duplicate; drop conflicting groups | Profile counts |
| D-G | Cross-dataset feature space (UNSW-NB15 uses different extraction tooling, so its features don't match CICFlowMeter's) | Defer to its own phase. Options: (a) run the same CICFlowMeter on UNSW-NB15 PCAPs (most rigorous, large download, Java toolchain); (b) map a small common feature subset (cheap, weak) | Feasibility check of (a) |
| D-H | NaN/Inf and scaling policy | §3.6 proposal | Profile results |

## 5. Next step (after these decisions are accepted)

Download the CNS2022 improved archive (328 MB) and record its SHA-256. Then implement
and test stage 1 (profiling) and run it. No cleaning or modelling happens until the
profile report has resolved the [VERIFY] items.
