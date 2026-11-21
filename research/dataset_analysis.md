# CICIDS2017 — Dataset Analysis

Status: literature review complete; **no data downloaded or measured yet**.
Every claim below is tagged with its source. Claims tagged **[VERIFY]** are things
we expect or suspect but have not confirmed; Phase 2 profiling must measure them.

## Sources

| Tag | Source | Access |
|---|---|---|
| S1 | Canadian Institute for Cybersecurity, *Intrusion Detection Evaluation Dataset (CIC-IDS2017)*, https://www.unb.ca/cic/datasets/ids-2017.html | read in full |
| S2 | I. Sharafaldin, A. H. Lashkari, A. A. Ghorbani, "Toward Generating a New Intrusion Detection Dataset and Intrusion Traffic Characterization", ICISSP 2018, pp. 108–116 | cited via S1/S3; full text not read |
| S3 | G. Engelen, V. Rimmer, W. Joosen, "Troubleshooting an Intrusion Detection Dataset: the CICIDS2017 Case Study", IEEE Security and Privacy Workshops (SPW/WTMC) 2021, pp. 7–12, doi:10.1109/SPW53761.2021.00009 | full text read |
| S3w | Companion site to S3: https://intrusion-detection.distrinet-research.be/WTMC2021/ (extended documentation, tools & dataset pages) | read |
| S4 | L. Liu, G. Engelen, T. Lynar, D. Essam, W. Joosen, "Error Prevalence in NIDS datasets: A Case Study on CIC-IDS-2017 and CSE-CIC-IDS-2018", IEEE CNS 2022, pp. 254–262 | companion site https://intrusion-detection.distrinet-research.be/CNS2022/ read; paper full text not read |
| S5 | CICFlowMeter documentation: https://github.com/ahlashkari/CICFlowMeter (README, ReadMe.txt) and https://www.unb.ca/cic/research/applications.html | read |
| S6 | D. Arp et al., "Dos and Don'ts of Machine Learning in Computer Security", USENIX Security 2022, pp. 3971–3988; https://dodo-mlsec.org/ | pitfall definitions read; full text not read |
| — | Lanvin et al., "Errors in the CICIDS2017 dataset and the significant differences in detection performances it makes", CRiSIS 2022 | **not accessed** (HAL blocks automated access); no claims taken from it |

---

## 1. What a network flow is

A flow is a **bidirectional** exchange of packets sharing one 5-tuple
{source IP, destination IP, source port, destination port, transport protocol}
within a time period [S3]. The first packet decides the *forward* direction
(source → destination); the reverse is *backward*, and most statistics are computed
per direction [S5].

A flow ends when [S3, S5]:
- it reaches the **flow timeout** — **120 s in CICIDS2017** [S3] (the tool docs only
  give 600 s as an example [S5]); or
- for TCP, the original CICFlowMeter sees **the first FIN packet** [S3].

Each flow becomes one CSV row: a fixed-length vector of statistics. **The model
never sees packets or payloads**, only these aggregates. That shapes everything:
a flow-based detector judges each connection on its own, with no context from
other flows [S3].

## 2. How CICFlowMeter builds features

CICFlowMeter (formerly ISCXFlowMeter) is a Java tool that reads PCAPs and emits
one CSV row per flow [S5]. Feature groups, documented in [S5]:

| Group | Examples | What it captures |
|---|---|---|
| Volume | Total Fwd/Bwd Packets, Total Length of Fwd/Bwd Packets | how much each side sent |
| Packet size stats | Fwd/Bwd Packet Length Min/Max/Mean/Std, Packet Length Variance, Average Packet Size, Fwd/Bwd Segment Size Avg | payload shape (e.g. tiny probes vs bulk transfer) |
| Timing | Flow Duration (µs), Flow/Fwd/Bwd IAT Mean/Std/Min/Max/Total | inter-arrival times; slow-rate DoS shows up here |
| Rates | Flow Bytes/s, Flow Packets/s, Fwd/Bwd Packets/s | derived from volume ÷ duration |
| TCP flags | FIN/SYN/RST/PSH/ACK/URG/CWR/ECE Flag Count, Fwd/Bwd PSH/URG Flags | handshake and teardown behaviour; scans |
| Headers / windows | Fwd/Bwd Header Length, Init Win bytes (fwd/bwd), Fwd Seg Size Min, Fwd Act Data Pkts | TCP stack and OS fingerprints |
| Bulk / subflow | Fwd/Bwd Bytes/Bulk Avg, Packet/Bulk Avg, Bulk Rate Avg; Subflow Fwd/Bwd Packets/Bytes | bulk-transfer behaviour (rules not documented [S5]) |
| Activity | Active/Idle Min/Mean/Max/Std | on/off periods within a flow |
| Identifiers | Flow ID, Source/Destination IP & Port, Protocol, Timestamp | **not behavioural features** (see §5.4) |

The documentation is incomplete [S5]:
- Units are given only for Flow Duration (µs).
- It doesn't say whether Std is the sample or population standard deviation.
- The "bulk", "subflow" and active/idle threshold rules are not defined.
- Feature counts disagree: the ReadMe lists 76 features, S1 says "more than 80",
  and S3 counts 83 excluding the label.

**[VERIFY]** the exact column list and units from the files themselves.

## 3. Attack categories and schedule

The capture runs from Monday 3 July 2017, 09:00, to Friday 7 July 2017, 17:00 [S1].

| Day | Traffic (official windows, S1) |
|---|---|
| Mon 3 Jul | Benign only |
| Tue 4 Jul | FTP-Patator (09:20–10:20), SSH-Patator (14:00–15:00) |
| Wed 5 Jul | DoS slowloris (09:47–10:10), DoS Slowhttptest (10:14–10:35), DoS Hulk (10:43–11:00), DoS GoldenEye (11:10–11:23), Heartbleed on port 444 (15:12–15:32) |
| Thu 6 Jul AM | Web Attack – Brute Force (09:20–10:00), XSS (10:15–10:35), SQL Injection (10:40–10:42) |
| Thu 6 Jul PM | Infiltration (several windows, 14:19–15:45), including an internal port scan from the compromised Vista host |
| Fri 7 Jul AM | Botnet ARES (10:02–11:02) |
| Fri 7 Jul PM | PortScan (13:55–15:29, firewall on and off), DDoS LOIT (15:56–16:16) |

The official time windows are inaccurate: S3 had to adjust them by hand to
reproduce the labels, and S4 found the SQL injection started about 5 minutes before
its published window. **Never label data from the S1 windows.**

**The testbed** [S1]:
- An attacker network (Kali 205.174.165.73; three Windows 8.1 hosts .69–.71 for DDoS).
- A victim network of 12 machines (Ubuntu, Windows 7/8.1/10/Vista, macOS), with the
  main web server at 192.168.10.50.
- Firewall at 205.174.165.80 / 172.16.0.1; DNS and domain controller at 192.168.10.3.
- Benign traffic comes from the B-Profile system: 25 simulated users over HTTP,
  HTTPS, FTP, SSH and email.

## 4. File organisation

S1 distributes [S1]:
- full PCAPs (11 GB Mon, 11 GB Tue, 13 GB Wed, 7.8 GB Thu, 8.3 GB Fri);
- `GeneratedLabelledFlows.zip`, which includes identifier columns;
- `MachineLearningCSV.zip`, the ML-ready CSVs;
- the user profiles.

S1 gives no row counts. S3 states "over 2.8 million traffic traces categorized into
15 classes".

**[VERIFY]**:
- the exact file names and the per-day / per-half-day split
  (we expect about 8 CSVs: Thursday and Friday split into morning and afternoon);
- which columns `MachineLearningCSV` drops relative to `GeneratedLabelledFlows`.

**Corrected releases** (relevant to the dataset-version decision in methodology.md):
- **WTMC2021** [S3w]: regenerated with a fixed CICFlowMeter, relabelled, and adds
  "Attempted" labels. https://intrusion-detection.distrinet-research.be/WTMC2021/Dataset/dataset.zip
  (CSVs regenerated in Oct–Nov 2021 for Idle, Bulk, Down/Up and flow-length fixes).
- **CNS2022 "improved"** [S4]: further label corrections and an
  `Attempted Category` column (-1 = not attempted, 0–6 = category).
  https://intrusion-detection.distrinet-research.be/CNS2022/Datasets/CICIDS2017_improved.zip
  (328 MB per the directory listing, dated 2023-04-27).
  **[VERIFY]** its columns, file layout, and whether it contains timestamps and IPs.

---

## 5. Data quality: documented problems vs. what we must verify

### 5.1 Flow-construction bugs (documented, S3)

1. **Early TCP termination and "TCP appendices".** The original CICFlowMeter ends a
   TCP flow on the *first* FIN, but under TCP a connection only closes when both
   sides have sent FIN. The remaining ACK and FIN packets then form a separate,
   meaningless flow that inherits the connection's label. **These appendices are
   25.9% of the whole dataset**, and for several attack classes they approach 50%
   [S3].
2. **Direction flips after a timeout.** When a TCP appendix times out, the next flow
   can come out with source and destination swapped. Labelling relies on direction,
   so these attack flows can end up labelled BENIGN [S3].
3. **RST is ignored** as a way to end a flow [S3].
4. **Feature extraction errors** [S3w]:
   - TCP flag counts were only incremented for the first packet, so in this dataset
     they are effectively 0 or 1.
   - Fwd/Bwd PSH and URG counts are wrong.
   - `Fwd Header Length` appears twice.

### 5.2 Labelling problems (documented)

- **Labels come only from IP, port and time window** [S3]. A flow between an attacker
  and a victim during an attack window is labelled malicious even if it carries no
  attack. In the original data, most Web Brute Force and XSS flows have **no forward
  payload**, roughly 2 of 3 Bot connections are rejected, and DoS classes include
  many failed connections. S3 relabels these as `X - Attempted`.
- **DoS Hulk is misimplemented** [S3]. The tool sends `Connection: close` instead of
  keep-alive, so the attack is ineffective. S3 recommends not using it as a malicious
  class. S4 keeps it as DoS Hulk but notes it is strictly "Attempted".
- **Further errors found in S4**:
  - The Infiltration "Cool disk – MAC" traffic was entirely missed by the original
    labelling.
  - Infiltration port scans outside the official window were labelled benign.
  - FTP-Patator contains a successful-login flow.
  - Some packets are out of order (SYN/ACK timestamped before SYN).
  - Identical LOIC flows also occur outside the DDoS window.
- **How much this matters (S3, Table I).** Flow counts per class, original → final:
  - Benign 2,271,326 → 1,823,964
  - DoS Hulk 230,124 → 158,469
  - DDoS 128,022 → 95,123
  - Web Brute Force 1,507 → 151
  - Web XSS 652 → 27
  - Bot 1,956 → 738
  - plus 447,362 Attempted flows

  Overall, "more than 20 percent of original traffic traces are reconstructed or
  relabelled" [S3]. The companion site [S3w] gives a later Benign count of 1,657,069,
  which excludes corrupted and NaN entries after the 2021 regenerations. The figure
  depends on the version, so **[VERIFY]** it against whichever version we use.
- **Attempted flows are not a class.** The authors say: "Under no circumstance should
  the 'Attempted' flows be treated as a separate label"; when in doubt, relabel them
  as Benign [S4].

### 5.3 Shortcut learning (documented, S3)

In the original data, a Random Forest told appendices of different classes apart
using `Fwd Header Len`, `Total Fwd Pkt`, `Min Seg Size Fwd` and `Init Win bytes Fwd`.
It separated appendices from real flows using `SYN Flag Count` and
`Bwd Pkt Len Mean`. None of these has any semantic link to the attacks [S3].

Bot detection in the original data relied on RST packets, so the model learned
"failed connection", not "botnet" [S3].

**Aggregate metrics hid all of this.** Weighted F1 was 0.99 for every dataset
version, while per-class F1 changed sharply. For example, XSS went from 0.18 to 0.79
and Bot from 0.60 to 0.99 [S3, Table II].

### 5.4 Identifier leakage (documented in part, partly our own reasoning)

- **Flow ID, Source/Destination IP, Source Port and Timestamp must be excluded from
  features** [S3]. Attackers have fixed IPs (e.g. 205.174.165.73), so a model can
  learn "this IP is bad" [S1, S6 "spurious correlations"].
- **Destination port** is "up for debate" [S3]. Our view: in this testbed it is
  strongly confounded. For example, Heartbleed targets port 444 only because of how
  the testbed was configured [S1]. We exclude it from the primary feature set and
  test it as an ablation.
- **Timestamps** stay in the data, because the temporal split needs them, but never
  as features.
- **[VERIFY]** whether behavioural features act as host fingerprints, such as
  `Init Win bytes` (OS-specific TCP window). This is a softer form of IP leakage;
  check it with feature importance and per-host error analysis.

### 5.5 Missing, infinite and malformed values

Documented:
- S3w excludes "corrupted entries and entries with NaN numeric features" from its
  counts.
- The duplicated `Fwd Header Length` column [S3w].

To measure ourselves; **[VERIFY]** all of these, since we found no accessible
source with numbers:
- NaN and ±Inf in the rate features (`Flow Bytes/s`, `Flow Packets/s`). We suspect
  they come from flows with zero duration (division by zero).
- Negative values (e.g. `Init Win bytes` = -1, negative IAT or duration).
- Leading whitespace in column names.
- Non-ASCII or mis-encoded characters in label strings (e.g. the dash in
  "Web Attack – …").
- Constant (zero-variance) columns.
- The timestamp format and timezone, and whether it is 12- or 24-hour.

### 5.6 Duplicates

S3 and S4 don't report duplicate-row counts. We couldn't access a reliable source
with numbers, so we quote none. **[VERIFY]**:
- **exact duplicates**: identical feature vectors, with identifiers excluded;
- **conflicting duplicates**: identical feature vectors with different labels;
- **cross-split duplicates**: a test row identical to a training row.

All three are expected to be common for scans and DoS, where one tool emits many
identical flows.

### 5.7 Class imbalance (documented, S3 Table I)

From the S3 final version:
- Benign: 1.82 M flows.
- Large attack classes: PortScan 159 k, DoS Hulk 158 k, DDoS 95 k.
- Five classes have fewer than 200 flows: Web BF 151, Infiltration 32, XSS 27,
  SQLi 12, Heartbleed 11.

A per-class test set for the tiny classes would hold only a handful of flows. S3
itself had 3 SQLi test flows, and all 3 were misclassified.

### 5.8 Limits of the collection environment (documented and reasoned)

- **Simulated traffic.** Benign traffic is generated by 25 scripted profiles [S1],
  and S3 notes the dataset keeps "basic limitations of a simulated environment".
  Real networks are far more varied, so false-positive rates measured here are
  likely optimistic (S6 "lab-only evaluation").
- **One configuration per attack tool.** S3 notes there is no information on whether
  attack tools were run with varied parameters. Detecting *these tool runs* does not
  mean detecting the attack class in general.
- **Attack prevalence is set by the schedule.** It does not reflect real-world base
  rates (S6 "base rate fallacy").
- **Each attack type occurs on a single day.** This limits which splits are possible
  (see methodology.md).
- **The capture is from 2017**, with that era's protocols, TLS versions and OSes.
